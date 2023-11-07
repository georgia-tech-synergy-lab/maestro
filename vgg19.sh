
cd tools/frontend

#python frameworks_to_modelfile_maestro.py --api_name keras --input_size 3,224,224 --model vgg19 --outfile vgg19_model.m

python modelfile_to_mapping.py --model_file vgg19_model.m --dataflow kcp_ws --outfile vgg19_kcp_ws.m

python modelfile_to_mapping.py --model_file vgg19_model.m --dataflow yxp_os --outfile vgg19_yxp_os.m

python modelfile_to_mapping.py --model_file vgg19_model.m --dataflow xp_ws --outfile vgg19_xp_ws.m

python modelfile_to_mapping.py --model_file vgg19_model.m --dataflow rs --outfile vgg19_rs.m

python modelfile_to_mapping.py --model_file vgg19_model.m --dataflow nlr --outfile vgg19_nlr.m

cd ../../

mkdir vgg19_results

./maestro --HW_file='data/hw/accelerator_2.m' \
          --Mapping_file='data/mapping/vgg19_kcp_ws.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg19_kcp_ws.csv vgg19_results/vgg19_kcp_ws.csv

./maestro --HW_file='data/hw/accelerator_2.m' \
          --Mapping_file='data/mapping/vgg19_rs.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg19_rs.csv vgg19_results/vgg19_rs.csv

./maestro --HW_file='data/hw/accelerator_2.m' \
          --Mapping_file='data/mapping/vgg19_xp_ws.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg19_xp_ws.csv vgg19_results/vgg19_xp_ws.csv

./maestro --HW_file='data/hw/accelerator_2.m' \
          --Mapping_file='data/mapping/vgg19_yxp_os.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg19_yxp_os.csv vgg19_results/vgg19_yxp_os.csv

./maestro --HW_file='data/hw/accelerator_2.m' \
          --Mapping_file='data/mapping/vgg19_nlr.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg19_nlr.csv vgg19_results/vgg19_nlr.csv
