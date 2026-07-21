sudo opae.io release -d 0000:01:00.1
sudo opae.io release -d 0000:01:00.2
sudo opae.io release -d 0000:01:00.3
sudo opae.io release -d 0000:01:00.4

aocl install $OFS_ASP_ROOT

aocl initialize acl0 ofs_iseries-dk_usm_noc
aocl initialize acl1 ofs_iseries-dk_usm_noc
aocl initialize acl2 ofs_iseries-dk_usm_noc
aocl initialize acl3 ofs_iseries-dk_usm_noc

sudo opae.io init -d 0000:01:00.1 $USER
sudo opae.io init -d 0000:01:00.2 $USER
sudo opae.io init -d 0000:01:00.3 $USER
sudo opae.io init -d 0000:01:00.4 $USER

