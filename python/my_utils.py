import os
from multiprocessing import shared_memory, resource_tracker
from typing import List
import traceback

import pandas as pd
import numpy as np

from HighFreqFileSystem import HDFData

import warnings
warnings.filterwarnings("ignore", category=UserWarning)


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
    D = HDFData('1day', log_level=0)
    df_data = D.get_data(date, date, [name], copy=True)

    df_data = df_data.set_index('securityid')
    df_data = df_data.loc[sec_list]
    res_list = df_data[name].tolist()
    D.close()
    return res_list


def save_shm_data(
    row_index: List[int], 
    col_index: List[int], 
    shm_names: List[str], 
    save_names: List[str], 
    dtype: str, 
    date: str,
    path_save: str
    ):
    date = pd.to_datetime(date).strftime('%Y%m%d')
    row_index = pd.to_datetime(
        date + pd.Series(row_index).astype(str).str.rjust(9, '0'), 
        format="%Y%m%d%H%M%S%f"
        )
    col_index = pd.Series(col_index).astype(str).str.rjust(6, '0')

    for i, name in enumerate(shm_names):
        try:
            shm = shared_memory.SharedMemory(name=name)
            resource_tracker.unregister(shm._name, 'shared_memory')
            shm_array = np.ndarray((len(row_index), len(col_index)), dtype=dtype, buffer=shm.buf)
            shm_df = pd.DataFrame(shm_array, index=row_index, columns=col_index)

            os.makedirs(os.path.join(path_save, date), exist_ok=True)
            shm_df.to_parquet(os.path.join(path_save, date, save_names[i] + '.parquet'))
        except:
            print(traceback.format_exc())
            continue
    return
