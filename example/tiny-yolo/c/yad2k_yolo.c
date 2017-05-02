#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "cqt_gen/cqt_gen.h"
#include "ya2k_yolo.h"


BOX out_boxes[YOLO_MAX_RESULT];
float out_scores[YOLO_MAX_RESULT];
int out_classes[YOLO_MAX_RESULT];

//yolo_eval_output
//float box_xy[YOLO_REGION_SIZE][YOLO_REGION_SIZE][YOLO_CLUSTERS][2];
//float box_wh[YOLO_REGION_SIZE][YOLO_REGION_SIZE][YOLO_CLUSTERS][2];
//float box_confidence[YOLO_REGION_SIZE][YOLO_REGION_SIZE][YOLO_CLUSTERS];
//float box_class_probs[YOLO_REGION_SIZE][YOLO_REGION_SIZE][YOLO_CLUSTERS][YOLO_CLASSES];

//決め打ちにしないとデバッガが落ちる
float box_xy[13][13][5][2];
float box_wh[13][13][5][2];
float box_confidence[13][13][5];
float box_class_probs[13][13][5][20];

//yolo_boxes_to_cornersの出力
BOX boxes_t [13][13][5];

//boxes, scores, classes

const char voc_class[YOLO_CLASSES][YOLO_BUFSIZE] = {
        "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat",
        "chair", "cow", "diningtable", "dog", "horse", "motorbike", "person",
        "pottedplant", "sheep", "sofa", "train", "tvmonitor"
};

const float voc_anchors[YOLO_CLUSTERS][2] = {{1.08, 1.19}, {3.42, 4.41}, {6.63, 11.38}, {9.42, 5.11}, {16.62, 10.52}};

//関数プロトタイプ
void yolo_head(void *predp);
float sigmoid(float x);
void yolo_boxes_to_corners(void);

void yolo_boxes_to_corners(void)
{
    int row, col, k;
    float box_mins0, box_mins1, box_maxes0, box_maxes1;
    for(row=0;row<YOLO_REGION_SIZE;row++) {
        for(col=0;col<YOLO_REGION_SIZE;col++) {
            for(k=0;k<YOLO_CLUSTERS;k++) {
                box_mins0 = box_xy[row][col][k][0] - (box_wh[row][col][k][0] / 2);
                box_mins1 = box_xy[row][col][k][1] - (box_wh[row][col][k][1] / 2);
                box_maxes0 = box_xy[row][col][k][0] + (box_wh[row][col][k][0] / 2);
                box_maxes1 = box_xy[row][col][k][1] + (box_wh[row][col][k][1] / 2);
                boxes_t[row][col][k].left   = box_mins0;
                boxes_t[row][col][k].top    = box_mins1;
                boxes_t[row][col][k].right  = box_maxes0;
                boxes_t[row][col][k].bottom = box_maxes1;
            }
        }
    }
}



float sigmoid(float x)
{
    return (float) (1.0 / (1 + exp(-x)));

}

//処理結果は、box_xy, box_wh, box_confidence, box_class_probs
void yolo_head(void *predp)
{
    int row, col;
    int k, i, idx_k;
    float data0, data1;
    float softmax_work[YOLO_CLASSES];
    float softmax_sum;
    float softmax_max;

    //配列の並びをKerasに合わせる。
    for(row=0;row<YOLO_REGION_SIZE;row++) {
        for(col=0;col<YOLO_REGION_SIZE;col++) {


            for(k=0;k<YOLO_CLUSTERS;k++) {
                //yoloの出力結果にアクセスするときは、idx_kを使う。
                idx_k = k * (YOLO_CLASSES + 5);
                printf("idx_k = %d\n", idx_k);
                fflush(stdout);

                //box_xy = sigmoid(feats[..., :2])
                data0 = conv2d_9_output[idx_k+0][row][col];
                data1 = conv2d_9_output[idx_k+1][row][col];
                box_xy[row][col][k][0] = (sigmoid(data0) + row) / YOLO_REGION_SIZE;
                box_xy[row][col][k][1] = (sigmoid(data1) + col) / YOLO_REGION_SIZE;

                //box_wh = np.exp(feats[..., 2:4])
                data0 = conv2d_9_output[idx_k+2][row][col];
                data1 = conv2d_9_output[idx_k+3][row][col];
                box_wh[row][col][k][0] = (float) ((exp(data0) * voc_anchors[k][0]) / YOLO_REGION_SIZE);
                box_wh[row][col][k][1] = (float) ((exp(data1) * voc_anchors[k][1]) / YOLO_REGION_SIZE);

                //box_confidence = sigmoid(feats[..., 4:5])
                data0 = conv2d_9_output[idx_k+4][row][col];
                box_confidence[row][col][k] = sigmoid(data0);

                //box_class_probs = softmax(feats[..., 5:])
                softmax_max = 0.0;
                //一回目のループでmaxを求める。
                for(i=0;i<YOLO_CLASSES;i++) {
                    data0 = conv2d_9_output[idx_k + 5 + i][row][col];
                    if (softmax_max < data0) {
                        softmax_max = data0;
                    }
                }
                //２回目のループでexpとsumを求める。
                softmax_sum = 0.0;
                for(i=0;i<YOLO_CLASSES;i++) {
                    data0 = conv2d_9_output[idx_k + 5 + i][row][col];
                    softmax_work[i] = (float) exp(data0 - softmax_max);
                    softmax_sum += softmax_work[i];
                }

                for(i=0;i<YOLO_CLASSES;i++) {
                    box_class_probs[row][col][k][i] = softmax_work[i] / softmax_sum;
                }
            }
        }
    }
}


int yolo_eval(void *predp, YOLO_PARAM *pp)
{
    assert(predp!=NULL);
    assert(pp!=NULL);
    assert(pp->classes==YOLO_CLASSES);
    yolo_head(predp);
    yolo_boxes_to_corners();

    return -1;
}
