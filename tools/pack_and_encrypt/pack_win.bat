rem python new_packer.py win 0 0
python copy_res.py ../release D:/LiveUpdate/patch_EHome true
python copy_res.py ../extend_res D:/LiveUpdate/patch_EHome false
python generate_patch.py D:/LiveUpdate/patch_EHome/

pause