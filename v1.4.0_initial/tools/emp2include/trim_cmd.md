
1. 生产trim 48 emp文件
hf_xtal_trim_patch 0x5b
hf_xtal_trim_patch 0x7e
hf_xtal_trim_patch 0x60

hf_xtal_trim_patch 0x63

hf_xtal_trim_patch 0x5d


hf_xtal_trim_patch 0x2d

2. 运行Python脚本产生适用于OTP的em9304_patches.c和.h
python emp2include.py -o OTP

hf_xtal_trim_patch 0x35