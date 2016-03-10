#!/bin/bash
# Usage:
# ./experiments/scripts/idnet_from_id_rand_proj.sh GPU NET DATASET [options args to {train,test}_net.py]
# DATASET is either pascal_voc or coco.
#
# Example:
# ./experiments/scripts/idnet_from_id_rand_proj.sh 0 VGG_CNN_M_1024 pascal_voc \
#   --set EXP_DIR foobar RNG_SEED 42 TRAIN.SCALES "[400, 500, 600, 700]"

set -x
set -e

export PYTHONUNBUFFERED="True"

GPU_ID=$1
NET=$2
NET_lc=${NET,,}
DATASET=$3

array=( $@ )
len=${#array[@]}
EXTRA_ARGS=${array[@]:3:$len}
EXTRA_ARGS_SLUG=${EXTRA_ARGS// /_}

case $DATASET in
  psdb)
    TRAIN_IMDB="psdb_train"
    TEST_IMDB="psdb_test"
    PT_DIR="psdb"
    ITERS=100000
    ;;
  *)
    echo "No dataset given"
    exit
    ;;
esac

LOG="experiments/logs/idnet_from_id_rand_proj_${NET}_${EXTRA_ARGS_SLUG}.txt.`date +'%Y-%m-%d_%H-%M-%S'`"
exec &> >(tee -a "$LOG")
echo Logging output to "$LOG"

time python2 ./tools/train_net.py --gpu ${GPU_ID} \
  --solver models/${PT_DIR}/${NET}/idnet/solver_rand_proj.prototxt \
  --weights data/id_pretrained_models/vgg16_ours_id_pretrained.caffemodel \
  --imdb ${TRAIN_IMDB} \
  --iters ${ITERS} \
  --cfg experiments/cfgs/idnet_from_id_rand_proj.yml \
  ${EXTRA_ARGS}

set +x
NET_FINAL=`grep -B 1 "done solving" ${LOG} | grep "Wrote snapshot" | awk '{print $4}'`
set -x

time python2 ./tools/test_idnet.py --gpu ${GPU_ID} \
  --gallery_def models/${PT_DIR}/${NET}/idnet/test_gallery.prototxt \
  --probe_def models/${PT_DIR}/${NET}/idnet/test_probe.prototxt \
  --net ${NET_FINAL} \
  --imdb ${TEST_IMDB} \
  --cfg experiments/cfgs/idnet_from_id_rand_proj.yml \
  ${EXTRA_ARGS}
