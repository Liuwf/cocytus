#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "cqt.h"
#include "numpy.h"

//see https://docs.scipy.org/doc/numpy-dev/neps/npy-format.html
const NUMPY_HEADER default_numpy_header = {0,0,0,CQT_DTYPE_NONE, 0, {0,0,0,0}};


//x93NUMPY
const unsigned char np_magic[6] = {0x93, 0x4E, 0x55, 0x4D, 0x50, 0x59};


int np_check_header(FILE *fp, NUMPY_HEADER *hp);
int np_parse_header_dic(char *buf, NUMPY_HEADER *hp);
void np_print_heaer_info(const NUMPY_HEADER *hp);


int load_from_numpy(void *dp, const char *numpy_fname, int size, NUMPY_HEADER *hp)
{

  FILE *fp;
  int ret;
  int size_from_shape;

  assert(dp!=NULL);
  assert(numpy_fname!=NULL);
  assert(size > 0);
  assert(hp!=NULL);

  fp = fopen(numpy_fname, "rb");
  if(fp==NULL) {
	printf("ERROR:cant'open %s\n", numpy_fname);
	return CQT_ERR_NO_FILE;

  }
  
  ret = np_check_header(fp, hp);
  if(ret != CQT_RET_OK) {
	printf("ERROR:numpy header error %s\n", numpy_fname);
	return ret;
  }

  printf("load from %s\n", numpy_fname);
  //np_print_heaer_info(hp);

  //引数のサイズと、numpyヘッダーのサイズを比較
  if(hp->shape[1] == 0) {
	  size_from_shape = hp->shape[0];
  } else if (hp->shape[2] == 0) {
	  size_from_shape = hp->shape[0] * hp->shape[1];
  } else if  (hp->shape[3] == 0) {
	  size_from_shape = hp->shape[0] * hp->shape[1] * hp->shape[2];
  } else {
	  size_from_shape = hp->shape[0] * hp->shape[1] *  hp->shape[2] *  hp->shape[3];
  }

  printf("size = %d, size_from_shape = %d\n", size, size_from_shape);

  if(size != size_from_shape) {
	printf("ERROR:numpy header error %s\n", numpy_fname);
	return CQT_NP_HEADER_ERR;
  }

  switch (hp->descr) {
  case CQT_FLOAT32:
	  assert(sizeof(float)==4);
	  fread(dp, 4, size, fp);
	  break;
  case CQT_UINT8:
	  fread(dp, 1, size, fp);
	  break;
  default:
	  printf("ERROR:numpy header error dscr = %d\n", hp->descr);
	  return CQT_NP_HEADER_ERR;
  }

  fclose(fp);

  return CQT_RET_OK;
}

//---------------------------------------------------------------------
// np_check_header
// 機能：numpyファイルのヘッダーチェックし、fpをデータの先頭までseekする 
//
// 引数
//  fp numpyファイルへのファイルポインタ。ファイル先頭を指していること
//      (fopen直後の状態であること)
//   hp ヘッダー情報の格納先
//
// 戻り値：
//   ヘッダー情報がおかしいとき、CQT_NP_HEADER_ERRを返す。
//   ヘッダー情報が問題ない場合は, CQT_RET_OKを返す。
int np_check_header(FILE *fp, NUMPY_HEADER *hp)
{
  unsigned char buf[CQT_NP_BUF_SIZE];
  int i;
  int size;
  int ret;

  //check magic number
  fread(&buf, 1, 6, fp);
  for(i=0;i<6;i++) {
	if(buf[i]!=np_magic[i]) {
	  return CQT_NP_HEADER_ERR;
	}
  }
  //version
  fread(&buf, 1, 2, fp);
  hp->major_version = buf[0];
  hp->minor_version = buf[1];
  
  //header_size 
  fread(&size, 2, 1, fp);
  hp->header_len = size;
  assert(hp->header_len < CQT_NP_BUF_SIZE-1);

  fread(&buf, 1, hp->header_len, fp);
  buf[hp->header_len] = '\0';

  ret = np_parse_header_dic((char *)buf, hp);
  if(ret != CQT_RET_OK) {
	return ret;
  }
	
  fseek(fp, 8+2+(hp->header_len), 0);

  return CQT_RET_OK;
}

//---------------------------------------------------------------------
// np_check_header
// 機能：numpyのヘッダーに含まれる辞書をパース氏、引数のhpに値を設定素する。
//
// 引数：
//   fp numpyファイルへのファイルポインタ。dic先頭を指していること
//      fpの値は更新される。
//   hp ヘッダー情報の格納先
//
// 戻り値：
//   ヘッダー情報がおかしいとき、CQT_NP_HEADER_ERRを返す。
//   ヘッダー情報が問題ない場合は, CQT_RET_OKを返す。
int np_parse_header_dic(char *buf, NUMPY_HEADER *hp)
{
  char *cp;
  const char *delimiter = ",{} ";
  int dim = 0;
  int ret;
  int size;
  
  //info 
  hp->shape[0] = 0;
  hp->shape[1] = 0;
  hp->shape[2] = 0;
  hp->shape[3] = 0;

  cp = strtok((char *)buf, delimiter);
  while(cp!=NULL) {
	if ((*cp == ' ') || *cp == '{') {
	  cp = strtok(NULL, delimiter);
	  continue;
	} 

	if(strstr(cp, "'descr':")!=NULL) {
	  cp = strtok(NULL, delimiter);
	  if(strstr(cp, "'<f4'")!=NULL) {
		hp->descr = CQT_FLOAT32;
	  } else if(strstr(cp, "'|u1'")!=NULL) {
		hp->descr = CQT_UINT8;
	  } else {
		printf("ERROR unkown descr %s\n", cp);
		return CQT_NP_HEADER_ERR;
	  }
	} else if(strstr(cp, "'fortran_order'")!=NULL) {
	  cp = strtok(NULL, delimiter);
	  //一文字目で判断
	  if(*cp=='F') {
		hp->fortran_order = false;
	  } else {
		printf("ERROR unsupported fortran_order %s\n", cp);
		return CQT_NP_HEADER_ERR;
	  }
	} else if(strstr(cp, "'shape':")!=NULL) {
	  do {
		cp = strtok(NULL, delimiter);
		//要素数が1の時は終了
		if(*cp==')') break;
		//(はスペースでつぶす
		if(*cp=='(') *cp = ' ';
		ret = sscanf(cp, "%d", &size);
		if(ret!=1) {
		  printf("ERROR unsupported shape %s\n", cp);
		  return CQT_NP_HEADER_ERR;
		}

		//hp->shapeがサイズ4の配列だから。
		if(dim>4) {
		  printf("ERROR unsupported shape %s\n", cp);
		  return CQT_NP_HEADER_ERR;
		}
		hp->shape[dim] = size;
		dim++;
	  } while(strstr(cp, ")")==NULL);

	}
	cp = strtok(NULL, delimiter);
  }


  return CQT_RET_OK;
}

void np_print_heaer_info(const NUMPY_HEADER *hp)
{
  printf("major_version=%d, ", hp->major_version);
  printf("minor_version=%d\n", hp->minor_version);
  printf("HEADER LEN=%d\n", hp->header_len);

  printf("descr=");
  switch(hp->descr) {
  case CQT_INT32:
	printf("int32");
	break;
  case  CQT_FLOAT32:
	printf("float32");
	break;
  case  CQT_QINT8:
	printf("qint8");
	break;
  case CQT_DTYPE_NONE:
	printf("none");
	break;
  default:
	printf("\nERROR unkown descr = %d", hp->descr);
  }
  printf("\n");

  if(hp->fortran_order) {
	printf("fortran_order=True\n");
  } else {
	printf("fortran_order=False\n");
  }

  printf("shape = (%d, %d, %d, %d)\n", hp->shape[0], hp->shape[1], hp->shape[2], hp->shape[3]);

}

int save_to_numpy(void *dp, const char *numpy_fname, NUMPY_HEADER *hp)
{
  
  FILE *fp;
  unsigned char buf[CQT_NP_BUF_SIZE];
  int i;
  int len;
  char *descr;
  char *type_float = "<f4";
  int size_from_shape;
  
  assert(dp!=NULL);
  assert(numpy_fname!=NULL);
  assert(hp!=NULL);
  assert(hp->shape[0] != 0);
  assert(hp->fortran_order == false);
	
	
  //引数のサイズと、numpyヘッダーのサイズを比較
  if(hp->shape[1] == 0) {
	  size_from_shape = hp->shape[0];
  } else if (hp->shape[2] == 0) {
	  size_from_shape = hp->shape[0] * hp->shape[1];
  } else if  (hp->shape[3] == 0) {
	  size_from_shape = hp->shape[0] * hp->shape[1] * hp->shape[2];
  } else {
	  size_from_shape = hp->shape[0] * hp->shape[1] *  hp->shape[2] *  hp->shape[3];
  }

  fp = fopen(numpy_fname, "wb");
  if(fp==NULL) {
	printf("ERROR:cant'open %s\n", numpy_fname);
	return CQT_ERR_NO_FILE;
  }

  //write magic number
  fwrite(np_magic, 1, 6, fp);


  //version
  fwrite(&(hp->major_version), 1, 1, fp);
  fwrite(&(hp->minor_version), 1, 1, fp);

  //size
  fwrite(&(hp->header_len), 2, 1, fp);

  //dictionary
  //padding
  for(i=0;i<CQT_NP_BUF_SIZE;i++) {
	buf[i] = ' ';
  }

  if(hp->descr == CQT_FLOAT32) {
	descr = type_float;
  } else {
	printf("ERROR unkown descr %d\n", hp->descr);
	return CQT_NP_HEADER_ERR;
  }
  
  //{'descr': '<f4', 'fortran_order': False, 'shape': (32,), }
  if(hp->shape[1] == 0) {
	len = sprintf((char *)buf, "{'descr': '%s', 'fortran_order': False, 'shape': (%d,), }", descr, hp->shape[0]);
  } else if(hp->shape[2] == 0) {
	len = sprintf((char *)buf, "{'descr': '%s', 'fortran_order': False, 'shape': (%d, %d), }", descr, hp->shape[0], hp->shape[1]);
  } else if(hp->shape[3] == 0) {
	len = sprintf((char *)buf, "{'descr': '%s', 'fortran_order': False, 'shape': (%d, %d, %d), }", descr, hp->shape[0], hp->shape[1], hp->shape[2]);
  } else {
	len = sprintf((char *)buf, "{'descr': '%s', 'fortran_order': False, 'shape': (%d, %d, %d, %d), }", descr, hp->shape[0], hp->shape[1], hp->shape[2], hp->shape[3]);
  }
  assert(len!=0);
  buf[len] = ' ';
  buf[hp->header_len-1] = '\n';
  
  fwrite(buf,1, hp->header_len, fp);

  fwrite(dp, 4, size_from_shape, fp);

  fclose(fp);
  
  return CQT_RET_OK;
}


