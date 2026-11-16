#general help

##pi hostname:
    wolfe-pi


##internet issues
    to change internet pass/username:
        sudo nvim /mnt/pi/etc/NetworkManager/system-connections/home-wifi.nmconnection
        ...
        then you can change and save with :wq
        ...
        then set required permissions:
            sudo chmod 600 /mnt/pi/etc/NetworkManager/system-connections/home-wifi.nmconnection
            sudo chown root:root /mnt/pi/etc/NetworkManager/system-connections/home-wifi.nmconnection
        ...
        unmount:
            sudo umount /mnt/pi


##finding the wolfe-pi
    ###mounting:
        sudo mkdir -p /mnt/pi
        sudo mount /dev/sdb2 /mnt/pi
        ...
        sudo umount /mnt/pi


    ###once found (2610 Roslyn Way)
        ssh ojzilla@192.168.1.237
        ...
        password:
            Nim8rod12!


##to update files on the pi:
    rsync -av --progress ./ ojzilla@192.168.1.237:~/pi-pedal/
