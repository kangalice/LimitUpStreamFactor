import os, sys
import subprocess
import argparse
from datetime import datetime
import logging
import json
import re

import pandas as pd

PATH_BASE = os.path.dirname(os.path.abspath(__file__))
PATH_ROOT = os.path.dirname(PATH_BASE)
sys.path.append(PATH_BASE)

from HighFreqFileSystem import HDFData

with open(os.path.join(PATH_ROOT, "config.json"), "r") as f:
    config = json.load(f)
    PATH_RAW = config['path_raw']
    PATH_CALENDAR = os.path.join(PATH_RAW)

def check_if_trade_date(date, path_calendar):
    '''检查某天是否是交易日'''
    md_trade_cal = pd.read_parquet(path_calendar)
    trade_dates = md_trade_cal.loc[
        ((md_trade_cal['IS_OPEN'] == 1) & 
        (md_trade_cal['EXCHANGE_CD'] == 'XSHE')), 
        'CALENDAR_DATE']
    trade_dates = pd.to_datetime(trade_dates).dt.strftime('%F')

    date = pd.to_datetime(date).strftime('%F')
    return date in trade_dates.values

def setup_logging():
    log_dir = os.path.join(PATH_ROOT, "logs")
    os.makedirs(log_dir, exist_ok=True)
    log_filepath = os.path.join(log_dir, "UpdateMin1Features.log")

    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(log_filepath),  # 日志写入文件
            logging.StreamHandler()             # 同时在控制台输出
        ]
    )

def search_list_pattern(file_path, start_marker, end_marker):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    vector_list = []
    pattern = re.escape(start_marker) + r'(.*?)' + re.escape(end_marker)
    match = re.search(pattern, content, re.DOTALL)
    if match:
        vector_list = re.findall(r'\"(.*?)\"', match.group(1))
    return vector_list

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--incre_port', type=int, nargs='?', default=2)
    parser.add_argument('--start_date', type=str, nargs='?', default='')
    parser.add_argument('--end_date', type=str, nargs='?', default='')
    parser.add_argument('--daily_update', type=int, nargs='?', default=0)
    parser.add_argument('--force_update', type=int, nargs='?', default=0)
    parser.add_argument('--feature_type', nargs='?')
    parser.add_argument('--save_type', type=str, nargs='?', default='parquet')
    args = parser.parse_args()

    incre_port = int(args.incre_port)
    daily_update = bool(int(args.daily_update))
    force_update = bool(int(args.force_update))

    if not daily_update and args.feature_type == None:
        print('please input feature_type')
        exit()

    feature_type = str(args.feature_type)
    save_type = str(args.save_type)

    setup_logging()
    logger = logging.getLogger(feature_type)

    path_bin = os.path.join(PATH_ROOT, 'bin')
    path_L2_ori = "/oss/tick_data/L2"

    # 区分文件更新与文件系统更新
    if save_type == 'parquet':
        with open(os.path.join(PATH_ROOT, "config.json"), "r") as f:
            config = json.load(f)
            path_save = config['path_factor_save']

        if daily_update:
            start_date = max(os.listdir(path_save))
            end_date = datetime.now().strftime('%F')
        else:
            start_date = pd.to_datetime(args.start_date).strftime('%F')
            end_date = pd.to_datetime(args.end_date).strftime('%F')
        
    elif save_type == 'fileSystem':
        # 先找到逻辑对应的所有因子名称和对应库
        path_ori_file = os.path.join(PATH_ROOT, 'src', feature_type + '.cpp')
        factor_list = search_list_pattern(
            path_ori_file, 
            "param.factor_names = std::vector<std::string>{", 
            "};"
            )
        use_factor_list = [feature_type + '/' + factor for factor in factor_list]

        save_data_list = search_list_pattern(
            path_ori_file, 
            "match_api->saveData(", 
            ");"
            )
        freq, asset = save_data_list[-1].split(',')
        D = HDFData(freq, asset=asset)

        if daily_update:
            # 在文件系统中搜索factor_time最早的时间作为start_time
            end_time_list = []
            for factor in use_factor_list:
                if D.check_factor_exist(factor):
                    factor_end_time = D.read_factor_time(factor).iloc[-1]
                    end_time_list.append(factor_end_time)
                else:
                    logger.error(f'some factor({factor}) not exist in file system, will not do daily update, please check')
                    exit()

            start_date = (min(end_time_list) + pd.Timedelta('1day')).strftime('%F')
            end_date = datetime.now().strftime('%F')
        else:
            if args.start_date == '' or args.end_date == '':
                logger.error(f'daily update is False, must input start_date and end_date')
                exit()
            
            start_date = pd.to_datetime(args.start_date).strftime('%F')
            end_date = pd.to_datetime(args.end_date).strftime('%F')
    else:
        logger.error(f'unrecognize save_type({save_type})')
        exit()

    D = HDFData('1day')
    trade_dates, _ = D.read_index()

    use_trade_dates = trade_dates[(trade_dates >= start_date) & (trade_dates <= end_date)].dt.strftime('%F').tolist()
    use_trade_dates = sorted(use_trade_dates)

    if use_trade_dates != []:
        logger.info(f'loop start date: {use_trade_dates[0]}  end date: {use_trade_dates[-1]}')
    else:
        logger.warning('no available trade_dates, will exit')
        exit()

    for date in use_trade_dates:
        try:
            if save_type == 'parquet' and date.replace('-', '') in os.listdir(path_save) and not force_update:
                logger.debug(f'Skipping {date} (already exists and force_update=False)')
                continue

            path_L2_check = os.path.join(path_L2_ori, date[:4])
            if date.replace('-', '') not in os.listdir(path_L2_check):
                logger.error(f'loss {date} L2 ori data')
                continue

            logger.info(f'Processing date: {date}')

            subprocess.Popen([os.path.join(path_bin, "offline_data"), date, f"{incre_port}"])
            subprocess.run([os.path.join(path_bin, feature_type), date, f"{incre_port}"], check=True)

            if save_type == 'parquet' and date.replace('-', '') not in os.listdir(path_save):
                logger.error(f'Some error occurred in processing {date}')
                
        except subprocess.CalledProcessError as e:
            logger.error(f'Subprocess failed for {date}: {e}', exc_info=True)
            
        except Exception as e:
            logger.error(f'Unexpected error processing {date}: {e}', exc_info=True)
            
        logger.info(f'date {date} saved succeed with feature {feature_type}' )
