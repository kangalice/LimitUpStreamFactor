import os
from multiprocessing import shared_memory, resource_tracker
from typing import List
import traceback
import time

import pandas as pd
import numpy as np
from joblib import Parallel, delayed

import warnings
warnings.filterwarnings("ignore", category=UserWarning)

SLEEP_TIME = 60
RETRY_TIMES = 2
L2_PRICE_COLS = (
    [f'ask{j}' for j in range(1, 11)] + 
    [f'bid{j}' for j in range(1, 11)] + 
    ['lastp', 'wavgbidpri', 'wavgaskpri']
    )
L2_VOL_COLS = (
    [f'askv{j}' for j in range(1, 11)] + 
    [f'bidv{j}' for j in range(1, 11)] + 
    [f'{j}' for j in ['tradnum', 'tradv', 'turnover', 'totalbidvol', 'totalaskvol']]
    )

def get_pit_list_secs(date: str, path_raw: str) -> list:
    '''获取pit的上市的股票列表，考虑到股票代码变更'''
    date = pd.to_datetime(date).strftime('%F')
    sec_chg_history = pd.read_parquet(os.path.join(path_raw, 'uqer_data/sec_chg_history.parquet'))
    sec_chg_history = sec_chg_history.loc[sec_chg_history['changType'] == u'代码变更']

    change_rec = pd.DataFrame(columns=['date', 'old_sec', 'now_sec'])
    i = 0
    for ticker, df in sec_chg_history.groupby('ticker'):
        if ticker[0] not in ['0', '3', '6']:
            continue
        for _, row in df.iterrows():
            if row['endDate'] != None:
                change_rec.loc[i] = [row['endDate'], row['value'], ticker]
                i += 1

    md_security = pd.read_parquet(os.path.join(path_raw, 'tonglian_db/md_security.parquet'))
    md_security['USE_LIST_DATE'] = md_security['LIST_DATE'].astype(str)
    md_security['USE_DELIST_DATE'] = md_security['DELIST_DATE'].fillna(date).astype(str)

    md_security = md_security.loc[(date >= md_security['USE_LIST_DATE']) & (date <= md_security['USE_DELIST_DATE'])]
    change_rec = change_rec.loc[change_rec['date'] > date]
    if not change_rec.empty:
        for _, row in change_rec.iterrows():
            md_security['TICKER_SYMBOL'] = md_security['TICKER_SYMBOL'].replace({row['now_sec']: row['old_sec']})
    sec_list = md_security['TICKER_SYMBOL'].unique().tolist()
    sec_list = [int(sec) for sec in sec_list]
    return sorted(sec_list)


def get_offline_hdf_data(name: str, date: str, sec_list: list) -> list:
    from HighFreqFileSystem import HDFData
    D = HDFData('1day', log_level=0)
    df_data = D.get_data(date, date, [name], copy=True)

    df_data = df_data.set_index('securityid')
    df_data = df_data.loc[sec_list]
    res_list = df_data[name].tolist()
    D.close()
    return res_list

def get_parquet_data(file_path: str, sec_list: list, row_is_sec: bool, col_is_sec: bool, fill_na: bool) -> list:
    df = pd.read_parquet(file_path)

    if row_is_sec:
        if not isinstance(df.index[0], np.number):
            df.index = [i[:6] for i in df.index]
        df.index = [int(i) for i in df.index]
        df = df.reindex(index=sec_list)
    if col_is_sec:
        if not isinstance(df.columns[0], np.number):
            df.columns = [i[:6] for i in df.columns]
        df.columns = [int(i) for i in df.columns]
        df = df.reindex(columns=sec_list)
    
    if fill_na:
        df = df.fillna(0)
    
    return df.values.reshape(-1).tolist(), df.shape[0], df.shape[1]

def _pre_process_l2_data(file_path, df):
    file_name = os.path.split(file_path)[-1]
    key = file_name.split('.')[0]

    if key in L2_PRICE_COLS:
        df = df.replace(0, np.nan)
    if key in L2_VOL_COLS:
        _mask = df.between_time('09:15:00', '09:25:00').index
        df.loc[_mask] = df.loc[_mask].replace(np.nan, 0)
    
    return df

def get_l2_parquet_data(file_path: str, sec_list: list, pre_process_offline_data: bool) -> list:
    df = pd.read_parquet(file_path)

    if not isinstance(df.columns[0], np.number):
        df.columns = [i[:6] for i in df.columns]
    df.columns = [int(i) for i in df.columns]
    df = df.reindex(columns=sec_list)
    
    if pre_process_offline_data:
        df = _pre_process_l2_data(file_path, df)
    
    return df.values.reshape(-1).tolist(), df.shape[0], df.shape[1]


def get_concept_matrix_data(file_path: str, sec_list: list, row_is_sec: bool, col_is_sec: bool, fill_na: bool) -> list:
    matrix = pd.read_parquet(file_path)
    matrix.drop(columns='concept_ID', inplace=True, errors='ignore')
    matrix.columns = [int(col) for col in matrix.columns]
    matrix = matrix.reindex(columns=sec_list)
    if fill_na:
        matrix = matrix.fillna(0)
    return matrix.values.reshape(-1).tolist(), matrix.shape[0], matrix.shape[1]


def get_hdfdata_data(start_date: str, end_date: str, py_factors_list: list, file_sys_prefix: str, py_sec_list: list, fill_na: bool):
    from HighFreqFileSystem import HDFData
    freq, asset = file_sys_prefix.split(',')
    D = HDFData(freq, asset=asset)
    dfs = D.get_data(start_date, end_date, py_factors_list, form='factor', pandas=True)
    
    data_list, row_len_list, col_len_list, factor_name_list = [], [], [], []
    for factor_name, df in dfs.items():
        use_df = df.reindex(columns=py_sec_list)
        if fill_na:
            use_df = use_df.fillna(0)
        data_list.append(use_df.values.reshape(-1).tolist())
        row_len_list.append(use_df.shape[0])
        col_len_list.append(use_df.shape[1])
        factor_name_list.append(factor_name)
    return data_list, row_len_list, col_len_list, factor_name_list


def _save_one_shm_parquet(name, row_index, col_index, dtype, path_save, date, save_name, align_cut_length):
    try:
        shm = shared_memory.SharedMemory(name=name)
        resource_tracker.unregister(shm._name, 'shared_memory')
        shm_array = np.ndarray((len(row_index), len(col_index)), dtype=dtype, buffer=shm.buf)
        shm_df = pd.DataFrame(shm_array, index=row_index, columns=col_index)

        if align_cut_length > 0:
            shm_df = shm_df.iloc[:, :-align_cut_length]

        path_file = os.path.join(path_save, date, save_name + '.parquet')
        os.makedirs(os.path.dirname(path_file), exist_ok=True)
        shm_df.to_parquet(path_file)
    except:
        print(traceback.format_exc())

def _get_shm_dfs(name, row_index, col_index, dtype, align_cut_length, temp_save=False):
    try:
        print(f'shm df {name} row_index({len(row_index)}), col_index({len(col_index)}), dtype({dtype})')
        shm = shared_memory.SharedMemory(name=name)
        resource_tracker.unregister(shm._name, 'shared_memory')
        shm_array = np.ndarray((len(row_index), len(col_index)), dtype=dtype, buffer=shm.buf)
        shm_df = pd.DataFrame(shm_array, index=row_index, columns=col_index, copy=True)

        if align_cut_length > 0:
            shm_df = shm_df.iloc[:, :-align_cut_length]

        if temp_save:
            shm_df.to_parquet(f'{name}.parquet')
        print(f'get {name} shm done, {shm_df}')
        return shm_df
    except:
        print(traceback.format_exc())

def save_shm_data(
    row_index: List[int], 
    col_index: List[int], 
    shm_names: List[str], 
    save_names: List[str], 
    dtype: str, 
    date: str,
    path_save: str,
    save_type: str,
    file_sys_prefix: str,
    mode: str,
    align_cut_length: int
    ):
    from HighFreqFileSystem import HDFData
    date = pd.to_datetime(date).strftime('%Y%m%d')
    row_index = pd.to_datetime(
        date + pd.Series(row_index).astype(str).str.rjust(9, '0'), 
        format="%Y%m%d%H%M%S%f"
        )
    col_index = pd.Series(col_index).astype(str).str.rjust(6, '0')
    njobs = min(5, len(shm_names))

    if save_type == 'parquet':
        Parallel(njobs)(delayed(_save_one_shm_parquet)(
            name, row_index, col_index, dtype, path_save, date, save_names[i], align_cut_length) 
            for i, name in enumerate(shm_names)
            )
        
    elif save_type == 'fileSystem':
        dfs = Parallel(njobs)(delayed(_get_shm_dfs)(
            name, row_index, col_index, dtype, align_cut_length) 
            for i, name in enumerate(shm_names)
            )
        print('get shm dfs done')

        # 稳健写入文件系统
        logic_name = '/'.join(save_names[0].split('/')[:-1])
        pure_factor_names = [name.split('/')[-1] for name in save_names]

        freq, asset = file_sys_prefix.split(',')
        D = HDFData(freq, asset=asset)

        for factor in save_names:
            if D.check_factor_exist(factor):
                flag_create_data = False
            else:
                flag_create_data = True
                break
        
        flag_success = False
        for try_time in range(1, RETRY_TIMES+1):
            try:
                if flag_create_data:
                    real_start_date = pd.to_datetime(row_index.iloc[0])
                    real_end_date = pd.to_datetime(row_index.iloc[-1])

                    register_data = [
                        [factor, freq, f'{logic_name}', 'Features', real_start_date.strftime('%F'), real_end_date.strftime('%F'), 'yyx']
                        for factor in pure_factor_names
                        ]
                    D.factor_register(register_data)

                    D.write_data(dfs, save_names)
                    if D.check_factor_exist(save_names[-1]):
                        print(f'create {logic_name} data success, {real_start_date} ~ {real_end_date}')
                        flag_success = True
                        D.close()
                        break
                    else:
                        print(f'create {logic_name} failed, please check! will retry after {SLEEP_TIME}s, try_time: {try_time}')
                        time.sleep(SLEEP_TIME)
                        continue
                else:
                    if mode == 'write':
                        D.write_data(dfs, save_names)
                    elif mode == 'fix':
                        D.fix_data(dfs, save_names, True)

                    print(f'update {logic_name} {row_index.iloc[0]} ~ {row_index.iloc[-1]} finish')
                    flag_success = True
                    D.close()
                    break
            except:
                print(traceback.format_exc())
                print(f'unexpected ERROR! will retry after {SLEEP_TIME}s, try_time: {try_time}')
                time.sleep(SLEEP_TIME)
                continue
        
        if not flag_success:
            print(f'unable to update {save_names[0]} in {RETRY_TIMES} retry_times, please check')
            return
        
    return
