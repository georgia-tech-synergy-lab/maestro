mkdir vgg16_results

./maestro --HW_file='data/hw/accelerator_1.m' \
          --Mapping_file='data/mapping/vgg16_ckp_ws.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg16_ckp_ws.csv vgg16_results/vgg16_ckp_ws.csv

./maestro --HW_file='data/hw/accelerator_1.m' \
          --Mapping_file='data/mapping/vgg16_nlr.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg16_nlr.csv vgg16_results/vgg16_nlr.csv


./maestro --HW_file='data/hw/accelerator_1.m' \
          --Mapping_file='data/mapping/vgg16_rs.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg16_rs.csv vgg16_results/vgg16_rs.csv

./maestro --HW_file='data/hw/accelerator_1.m' \
          --Mapping_file='data/mapping/vgg16_xp_ws.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg16_xp_ws.csv vgg16_results/vgg16_xp_ws.csv

./maestro --HW_file='data/hw/accelerator_1.m' \
          --Mapping_file='data/mapping/vgg16_yxp_os.m' \
          --print_res=true \
          --print_res_csv_file=true \
          --print_log_file=false \

mv vgg16_yxp_os.csv vgg16_results/vgg16_yxp_os.csv