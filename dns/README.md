# DNS
Use virtual box

Make sure the drives are intact and that the vms are using an internal network net5761

Run this command:
```
vboxmanage dhcpserver add --server-ip=135.23.161.254 --lower-ip=135.23.161.1 --upper-ip=135.23.161.64 --netmask=255.255.255.0 --set-opt=6 135.23.161.128 --enable --network=net5761
```

run client/main on dns-client with sudo for dhcp alterations

run proxy/main on dns-proxy

have dns-nameserver and dns-server on standby

## Links
Virtual Machines : https://drive.google.com/file/d/1WOqB7AG_XzRNfsoQTIw-fJa-xDm-hBlB/view?usp=drive_link

Preview : https://drive.google.com/file/d/1R8OUp47u7dLGX47Jr3Aq6O-2BKtEr3U3/view?usp=drive_link
