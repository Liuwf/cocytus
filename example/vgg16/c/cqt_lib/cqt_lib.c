// compileを通すためのダミーファイル
#include "cqt.h"
#include "cqt_net.h"

#include <math.h>
#include <assert.h>


int CQT_LeakyReLU_if_of(CQT_LAYER *lp, void *inp, void *outp)
{
    return 1;

}

int CQT_Conv2D_same_1x1_if_wf_wf_of(CQT_LAYER *lp, void *inp, void *outp)
{
    return 1;
}
