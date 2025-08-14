#!/bin/bash
path=$(dirname $(readlink -f "$0"))
path_root=$(dirname $path)
python=/home/yyx/miniconda3/envs/python3.9.12/bin/python

group1='Basic Cancel LOBstatus OrderSizeAbs TradeSizeAbs'
group2='Cancel'

daily_update=`eval echo "$2"`
save_type='parquet'
incre_port=0

for key in `eval echo '$'"group$1"`
do
    echo "$key, Port: $incre_port"
    if [[ $daily_update -eq 1 ]]; then
        $python -u $path/UpdateMin1Features.py --incre_port=$incre_port --daily_update=1 --feature_type=$key --save_type=$save_type > $path_root/logs/$key.nohup 2>&1
    else
        $python -u $path/UpdateMin1Features.py --incre_port=$incre_port --start_date=2025-08-04 --end_date=2025-08-04 --feature_type=$key --save_type=$save_type > $path_root/logs/$key.nohup 2>&1
    fi
    incre_port=$((incre_port + 2))
done
