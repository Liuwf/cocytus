import os
import shutil
import datetime

class CGenerator:
    def __init__(self, compiler):
        self.compiler = compiler
        self.config = self.compiler.config

    def generate(self):
        """
        Cソースファイルを作成する
        """
        # ディレクトリの作成
        target_dir = self.config['Cocyuts']['output_dir']
        create_c_dir(target_dir)

        # ヘッダーファイルの作成、コピー
        template_dir = self.config['Cocyuts']['c_lib_dir']

        self.generate_hedarfiles(target_dir, template_dir)

        # Cソースの作成
        target_dir = self.config['Cocyuts']['output_dir']
        self.generate_cqt_gen(target_dir)

        # ライブラリの作成
        self.generate_cqt_lib()

    def generate_hedarfiles(self, target_dir, template_dir):
        """
        template_dirから、target_dirへヘッダファイルをコピーする。
        :param target_dir: str
        :param template_dir: str
        :return:
        """
        headers = ['cqt.h', 'cqt_type.h', 'cqt_keras.h', 'numpy.h', 'cqt_net.h']
        for h in headers:
            shutil.copy(os.path.join(template_dir, h),
                        os.path.join(target_dir, 'inc'))


    def generate_cqt_gen(self, target_dir):
        """
        target_dirで指定されたディレクトリ/cqt_gen以下にcqt_gen.hとcqt_gen.cのファイルを作成する。
        :param target_dir: str
        :return:
        """
        cqt_gen_h_path = os.path.join(target_dir, 'cqt_gen', 'cqt_gen.h')
        print("making %s" % cqt_gen_h_path)
        cqt_gen_h = CqtGenH(cqt_gen_h_path, self.compiler)
        cqt_gen_h.generate()

        cqt_gen_c_path = os.path.join(target_dir, 'cqt_gen', 'cqt_gen.c')
        print("making %s" % cqt_gen_c_path)
        cqt_gen_c = CqtGenC(cqt_gen_c_path, self.compiler)
        cqt_gen_c.generate()


    def generate_cqt_lib(self):
       """
        ## Ｃライブラリ(cqt_lib)
        ### cqt_lib.h
        ### cqt_lib.c
        ### numpy.c
       """



class CFile:
    def __init__(self, file, compier):
        self.file = file
        self.fp = open(file, 'w')
        self.compiler = compier

    def __del__(self):
        self.fp.close()

    def wr(self, s):
        """
        ファイルに文字列を書き込む
        :param s: str
        :return:
        """
        self.fp.write(s)

    def wr_file_header(self):
        """
        自走生成されるファイルにつけられるヘッダー。
        :return:
        """
        # TODO コピーライトやコキュートスのバージョンを追加したい
        today = datetime.date.today()
        self.fp.write("//----------------------------------------------------------------------------------------------------\n")
        self.fp.write("// This file is automatically generated.\n")
        self.fp.write("// %s\n" % today.strftime('%Y/%m/%d %H:%M:%S'))
        self.fp.write("//----------------------------------------------------------------------------------------------------\n")

    def wr_include(self, file, stdlib=False):
        """
        include分を挿入する。
        stdlib = True　の時は #include <file>　形式を、stdlib = Falseの時は
        #include "file" 形式で書き込む。

        :param file: str
        :param stdlib: bool
        :return:
        """
        if stdlib:
            self.wr('#include <%s>\n' % file)
        else:
            self.wr('#include "%s"\n' % file)

    def cr(self):
        """
        改行の挿入
        :return:
        """
        self.wr('\n')

    def write_cqt_network(self, scope=None):
        """
        コキュートスニューラルネットの定義を行う。
        scopeを変更する場合は、scope引数に"extern"もしくは"static"を指定する。
        """
        name = self.compiler.get_model_name()
        scope_s = add_space(scope)

        self.wr('// cocytus network\n')
        self.wr('%sCQT_NET %s;\n' % (scope_s, name))


    def wr_layer_defination(self, scope=None):
        """
        レイヤー変数の定義を行う。
        scopeを変更する場合は、scope引数に"extern"もしくは"static"を指定する。
        :return:
        """
        self.wr("//Layers\n")
        model_config = self.get_config()
        for l in model_config['layers']:
            name = l['name']
            class_name = l['class_name']
            scope_s = add_space(scope)
            s = "%sLY_%s %s;\n" % (scope_s, class_name, name)

            self.wr(s)



    def wr_weight_defination(self, scope=None):
        """
        weight変数の定義を行う。
        scopeを変更する場合は、scope引数に"extern"もしくは"static"を指定する。
        :return:
        """
        self.wr("//weights\n")
        model_config = self.get_config()
        for l in model_config['layers']:
            name = l['name']
            class_name = l['class_name']
            if class_name == 'Conv2D':
                layer_detal = self.compiler.get_cqt_layer_obj(name)
                w_shape = layer_detal.get_Wshape()
                w_name, w_nph_name, b_name, b_nph_name = layer_detal.get_conv2d_weight_variable_name()
                w_dim_s = dim_str_from_keras_4d_shape(w_shape)
                b_dim_s = dim_str_from_keras_4d_shape_bias(w_shape)
                scope_s = add_space(scope)

                w_type = layer_detal.get_weight_type_str()

                self.wr('%sNUMPY_HEADER %s;\n' % (scope_s, w_nph_name))
                self.wr('%sNUMPY_HEADER %s;\n' % (scope_s, b_nph_name))
                self.wr("%s%s %s%s;\n" % (scope_s, w_type, w_name, w_dim_s))
                self.wr("%s%s %s%s;\n" % (scope_s, w_type, b_name, b_dim_s))
            elif class_name == 'Dense':
                layer_detal = self.compiler.get_cqt_layer_obj(name)
                w_shape = layer_detal.get_Wshape()
                input_dim = w_shape[0]
                output_dim = w_shape[1]
                scope_s = add_space(scope)

                w_name, w_nph_name, b_name, b_nph_name = layer_detal.get_conv2d_weight_variable_name()

                w_type = layer_detal.get_weight_type_str()

                self.wr('%sNUMPY_HEADER %s;\n' % (scope_s, w_nph_name))
                self.wr('%sNUMPY_HEADER %s;\n' % (scope_s, b_nph_name))
                self.wr("%s%s %s[%d][%d];\n" % (scope_s, w_type, w_name, output_dim, input_dim))
                self.wr("%s%s %s[%s];\n" % (scope_s, w_type, b_name, output_dim))


    def wr_output_defination(self, scope=None):
        """
        output変数の定義を行う。
        scopeを変更する場合は、scope引数に"extern"もしくは"static"を指定する。
        :return:
        """
        self.wr("//outputs\n")
        model_config = self.get_config()
        for l in model_config['layers']:
            name = l['name']
            class_name = l['class_name']
            layer_detal = self.compiler.get_cqt_layer_obj(name)
            o_shape = layer_detal.get_output_shape()
            dim_s = dim_str_from_keras_4d_shape_output(o_shape)

            o_name = layer_detal.get_output_variable_name()
            o_type = layer_detal.get_output_type_str()
            scope_s = add_space(scope)

            self.wr('%s%s %s%s;\n' % (scope_s, o_type, o_name, dim_s))

    def get_config(self):
        """
        Keras Modelのコンフィグ情報を返す。
        :return:
        """
        return self.compiler.model.get_config()


class CqtGenH(CFile):
    def __init__(self, file, compiler):
        super().__init__(file, compiler)

    def __del__(self):
        super().__del__()

    def generate(self):
        self.wr_file_header()
        self.wr_include('stdio.h', stdlib=True)
        self.wr_include('cqt.h')
        self.wr_include('cqt_net.h')
        self.cr()

        self.wr('CQT_NET* cqt_init(void);\n')
        self.wr('int cqt_load_weight_from_files(CQT_NET* np, const char *path);\n')
        self.wr('int cqt_run(CQT_NET* np, void *dp);\n')
        self.cr()

        self.write_cqt_network(scope='extern')
        self.cr()

        self.wr_layer_defination(scope='extern')
        self.cr()

        self.wr_weight_defination(scope='extern')
        self.cr()

        self.wr_output_defination(scope='extern')
        self.cr()


        self.fp.write('\n')


class CqtGenC(CFile):
    def __init__(self, file, compiler):
        super().__init__(file, compiler)

    def __del__(self):
        super().__del__()

    def generate(self):
        self.wr_file_header()
        self.wr_include('cqt_gen.h')
        self.cr()

        self.write_cqt_network()
        self.cr()

        self.wr_layer_defination()
        self.cr()

        self.wr_weight_defination()
        self.cr()

        self.wr_output_defination()
        self.cr()

        self.wr('CQT_NET* cqt_init(void) { return NULL;};\n')
        self.wr('int cqt_load_weight_from_files(CQT_NET* np, const char *path) { return 0;}\n')
        self.wr('int cqt_run(CQT_NET* np, void *dp) {return 0;}\n')
        self.cr()


def create_c_dir(tdir):
    """
    tdir以下に以下のディレクトリを作成する。
    inc, cqt_gen, cqt_lib
    :param tdir: str
    :return:
    """
    dirs = ['inc', 'cqt_gen', 'cqt_lib']
    for d in dirs:
        path = os.path.join(tdir, d)
        if not os.path.isdir(path):
            os.makedirs(path)


def dim_str_from_keras_4d_shape(shape):
    assert(len(shape)==4)
    if shape[3] is None:
        return "[%d][%d][%d]" % (shape[2], shape[1], shape[0])
    else:
        return "[%d][%d][%d][%d]" % (shape[3], shape[2], shape[1], shape[0])


def dim_str_from_keras_4d_shape_output(shape):
    if len(shape) == 4:
        return "[%d][%d][%d]" % (shape[3], shape[2], shape[1])
    elif len(shape) == 3:
        return "[%d][%d]" % (shape[2], shape[1])
    elif len(shape) == 2:
        return "[%d]" % shape[1]
    else:
        return ""


def dim_str_from_keras_4d_shape_bias(shape):
    assert(len(shape)==4)
    if (shape[3] is None) and (shape[2] is None) and (shape[1] is None):
        return "[%d]" % shape[0]
    elif (shape[3] is None) and (shape[2] is None):
        return "[%d]" % shape[1]
    elif shape[3] is None:
        return "[%d]" % shape[2]
    else:
        return "[%d]" % shape[3]


def add_space(s):
    """
    引数の文字列が空文字列でなかったら、末尾にスペースを追加した文字列を返す。
    引数が空文字列の場合は、空文字列を返す。
    :param s: str
    :return: str
    """
    if s is None:
        return  ''
    else:
        return s + ' '
