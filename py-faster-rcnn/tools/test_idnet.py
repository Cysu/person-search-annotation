#!/usr/bin/env python

# --------------------------------------------------------
# Fast R-CNN
# Copyright (c) 2015 Microsoft
# Licensed under The MIT License [see LICENSE for details]
# Written by Ross Girshick
# --------------------------------------------------------

"""Test a Fast R-CNN network on an image database."""

import _init_paths
from fast_rcnn.test_gallery import test_net_on_gallery_set
from fast_rcnn.test_probe import test_net_on_probe_set
from fast_rcnn.config import cfg, cfg_from_file, cfg_from_list, get_output_dir
from datasets.factory import get_imdb
from utils import unpickle
import caffe
import argparse
import pprint
import time, os, sys
import numpy as np
from scipy.io import loadmat
from sklearn.metrics import average_precision_score

def _compute_iou(a, b):
    x1 = max(a[0], b[0])
    y1 = max(a[1], b[1])
    x2 = min(a[2], b[2])
    y2 = min(a[3], b[3])
    inter = max(0, x2 - x1) * max(0, y2 - y1)
    union = (a[2] - a[0]) * (a[3] - a[1]) + (b[2] - b[0]) * (b[3] - b[1]) - inter
    return inter * 1.0 / union

def reid_eval(output_dir, imgmat, protoc, test_images):
    gallery_det = unpickle(os.path.join(output_dir, 'gallery_detections.pkl'))
    gallery_feat = unpickle(os.path.join(output_dir, 'gallery_features.pkl'))
    gallery_det = gallery_det[1]
    gallery_feat = gallery_feat[1]
    probe_feat = unpickle(os.path.join(output_dir, 'probe_features.pkl'))

    imname_to_boxes_pids = {}
    for imname, boxes, pids in imgmat:
        imname = str(imname[0])
        boxes = boxes.astype(np.int32)
        pids = pids.astype(np.int32).ravel()
        imname_to_boxes_pids[imname] = (boxes, pids)

    assert len(gallery_det) == len(gallery_feat)
    imname_to_det_feat = {}
    for imname, det, feat in zip(test_images, gallery_det, gallery_feat):
        scores = det[:, 4].ravel()
        inds = np.where(scores >= 0.8)[0]
        if len(inds) > 0:
            det = det[inds]
            feat = feat[inds]
            imname_to_det_feat[imname] = (det, feat)

    num_probe = protoc['gallery'].shape[0]
    assert len(probe_feat) == num_probe
    aps = []
    top1_acc = []
    for i in xrange(num_probe):
        y_true, y_score = [], []
        feat_p = probe_feat[i][np.newaxis, :]
        pid = int(protoc['query'][i, 0][0].squeeze())
        count_gt = 0
        count_tp = 0
        for gallery_imname in protoc['gallery'][i]:
            gallery_imname = str(gallery_imname[0])
            # check if the gallery image contain gt
            boxes, pids = imname_to_boxes_pids[gallery_imname]
            inds = np.where(pids == pid)[0]
            count_gt += (len(inds) > 0)
            if gallery_imname not in imname_to_det_feat: continue
            det, feat_g = imname_to_det_feat[gallery_imname]
            dis = np.sum((feat_p - feat_g) ** 2, axis=1)
            label = np.zeros(len(dis), dtype=np.int32)
            if len(inds) > 0:
                gt = boxes[inds[0]].copy()
                w, h = gt[2], gt[3]
                gt[2] += gt[0]
                gt[3] += gt[1]
                thresh = min(0.5, (w * h * 1.0) / ((w + 10) * (h + 10)))
                inds = np.argsort(dis)
                dis = dis[inds]
                # set the label of the first box matched to gt to 1
                for j, roi in enumerate(det[inds, :4]):
                    if _compute_iou(roi, gt) >= thresh:
                        label[j] = 1
                        count_tp += 1
                        break
            y_true.extend(list(label))
            y_score.extend(list(-dis))
        assert count_tp <= count_gt
        recall_rate = count_tp * 1.0 / count_gt
        ap = average_precision_score(y_true, y_score) * recall_rate
        if not np.isnan(ap):
            aps.append(ap)
        else:
            aps.append(0)
        maxind = np.argmax(y_score)
        top1_acc.append(y_true[maxind])

    print 'mAP: {:.2%}'.format(np.mean(aps))
    print 'top-1: {:.2%}'.format(np.mean(top1_acc))

def parse_args():
    """
    Parse input arguments
    """
    parser = argparse.ArgumentParser(description='Test a Fast R-CNN network')
    parser.add_argument('--gpu', dest='gpu_id', help='GPU id to use',
                        default=0, type=int)
    parser.add_argument('--gallery_def',
                        help='prototxt file defining the gallery network',
                        default=None, type=str)
    parser.add_argument('--probe_def',
                        help='prototxt file defining the probe network',
                        default=None, type=str)
    parser.add_argument('--net', dest='caffemodel',
                        help='model to test',
                        default=None, type=str)
    parser.add_argument('--cfg', dest='cfg_file',
                        help='optional config file', default=None, type=str)
    parser.add_argument('--wait', dest='wait',
                        help='wait until net file exists',
                        default=True, type=bool)
    parser.add_argument('--imdb', dest='imdb_name',
                        help='dataset to test',
                        default='voc_2007_test', type=str)
    parser.add_argument('--comp', dest='comp_mode', help='competition mode',
                        action='store_true')
    parser.add_argument('--set', dest='set_cfgs',
                        help='set config keys', default=None,
                        nargs=argparse.REMAINDER)
    parser.add_argument('--vis', dest='vis', help='visualize detections',
                        action='store_true')
    parser.add_argument('--num_dets', dest='max_per_image',
                        help='max number of detections per image',
                        default=100, type=int)
    parser.add_argument('--feat_blob',
                        help='name of the feature blob to be extracted',
                        default='feat')

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    args = parser.parse_args()
    return args

if __name__ == '__main__':
    args = parse_args()

    print('Called with args:')
    print(args)

    if args.cfg_file is not None:
        cfg_from_file(args.cfg_file)
    if args.set_cfgs is not None:
        cfg_from_list(args.set_cfgs)

    cfg.GPU_ID = args.gpu_id

    print('Using config:')
    pprint.pprint(cfg)

    while not os.path.exists(args.caffemodel) and args.wait:
        print('Waiting for {} to exist...'.format(args.caffemodel))
        time.sleep(10)

    caffe.set_mode_gpu()
    caffe.set_device(args.gpu_id)
    net = caffe.Net(args.gallery_def, args.caffemodel, caffe.TEST)
    net.name = os.path.splitext(os.path.basename(args.caffemodel))[0]

    imdb = get_imdb(args.imdb_name)
    imdb.competition_mode(args.comp_mode)
    if not cfg.TEST.HAS_RPN:
        imdb.set_proposal_method(cfg.TEST.PROPOSAL_METHOD)

    test_net_on_gallery_set(net, imdb, args.feat_blob,
                            max_per_image=args.max_per_image, vis=args.vis)

    # hard coded here to load protocols
    root_dir = imdb._root_dir
    images_dir = imdb._data_path
    protoc = loadmat(os.path.join(root_dir, 'protocols/test_gs_100.mat'))

    imgmat = loadmat(os.path.join(root_dir, 'image.mat'))['image'].squeeze()
    imname_to_boxes = {}
    imname_to_pids = {}
    for imname, boxes, pids in imgmat:
        imname = str(imname[0])
        boxes = boxes.astype(np.int32)
        boxes[:, 2] += boxes[:, 0]
        boxes[:, 3] += boxes[:, 1]
        pids = pids.astype(np.int32).ravel()
        imname_to_boxes[imname] = boxes
        imname_to_pids[imname] = pids

    # construct the probe images and rois
    probe_images = []
    probe_rois = []
    for item in protoc['query'].squeeze():
        pid = item[0][0, 0]
        imname = str(item[1][0])
        boxes, pids = imname_to_boxes[imname], imname_to_pids[imname]
        ind = np.where(pids == pid)[0]
        assert len(ind) == 1
        roi = boxes[ind, :]
        probe_images.append(os.path.join(images_dir, imname))
        probe_rois.append(roi)

    # extract features on probe rois
    net = caffe.Net(args.probe_def, args.caffemodel, caffe.TEST)
    net.name = os.path.splitext(os.path.basename(args.caffemodel))[0]
    output_dir = get_output_dir(imdb, net)
    test_net_on_probe_set(net, probe_images, probe_rois, args.feat_blob,
                          output_dir)

    # do the re-id evaluation
    reid_eval(output_dir, imgmat, protoc, imdb.image_index)