import re
import argparse
import os.path
from argparse import RawTextHelpFormatter

def get_dataflow_filename(model_name, layer_number):
    model_name_without_extension = os.path.splitext(model_name)[0]
    model_name_without_suffix = re.sub(r'_model$', '', model_name_without_extension)
    return f'./dataflow/{model_name_without_suffix}_{layer_number}.m'


if __name__ == "__main__":
    parser = argparse.ArgumentParser(formatter_class=RawTextHelpFormatter)
    parser.add_argument('--model_file', type=str, default="dnn_model", help="<name of your model file with layer specs>")
    parser.add_argument('--outfile', type=str, default="out.m", help='output file name')
    opt = parser.parse_args()
    print('Begin processing')
    base_path = '../../data/'

    if os.path.exists(base_path + 'model/' + opt.model_file):
        with open(base_path + 'mapping/' + opt.outfile, "w") as fo:
            with open('./dataflow/'+ 'dpt.m' , "r") as fdpt:
                with open(base_path + 'model/' + opt.model_file, "r") as fm:
                    dsconv = 0
                    i = 1
                    for line in fm:
                        if(re.search("DSCONV",line)):
                                dsconv = 1
                        if(re.search("Dimensions",line)):
                            fo.write(line)
                            if(dsconv):
                                    fdpt.seek(0)
                                    fo.write(fdpt.read())
                            else:
                                dataflow_file = get_dataflow_filename(opt.model_file, i)
                                with open(dataflow_file, "r") as fd:
                                    fd.seek(0)
                                    fo.write(fd.read())
                            i+=1
                            dsconv = 0
                        else:
                            fo.write(line)

        print("Mapping file created")
    else:
        print("Model file not found, please provide one")
