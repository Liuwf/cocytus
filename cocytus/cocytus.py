#!/bin/env python3
import sys
import configparser
import keras

class dummy():
    def __init__(self):
        pass


def open_inifile(ini_file):
    """
    inifileをオープンし、パーサーを返す。
    :param ini_file:str
    :return:ConfigParser
    """
    print("INI:open %s" % ini_file)
    config = configparser.ConfigParser()
    config.read(ini_file)
    return config


def check_config(config):
    """
    configの必須データを確認する。
    必須出たが存在しないとｋは、エラーを表示してプログラムを終了する。
    :param config:ConfigParser
    :return:bool
    """
    cqt_options = ['keres_json', 'output_dir']
    ret = True

    for opt in cqt_options:
        try:
            config.get('Cocyuts', opt)
        except (configparser.NoSectionError, configparser.NoOptionError):
            print("ERROR:no entry [%s] %s" % ('Cocyuts', opt))
            ret = False
    return ret


def main(argv):
    # TODO オプションの処理作成
    if len(argv) != 2:
        print('usage:python3 cocytus.py ini')
        sys.exit()
    ini_file = argv[1]
    config = open_inifile(ini_file)

    # TODO ここにオプションで上書きの処理を入れる。
    if not check_config(config):
        print("ERROR:ini file %s" % ini_file)
        sys.exit(1)

if __name__ == '__main__':
    main(sys.argv)





