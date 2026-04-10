# DAO Induction Attack Experiment Setup

## 1. Recover All Devices

``` bash
nrfjprog --recover
```

------------------------------------------------------------------------

## 2. Border Router Setup

Navigate to the border router directory and run:

``` bash
sudo make TARGET=nrf52840 BOARD=dk distclean
sudo make TARGET=nrf52840 BOARD=dk border-router.upload
```

------------------------------------------------------------------------

## 3. Hello World Devices

### Attacker Node

``` bash
sudo make TARGET=nrf52840 BOARD=dk distclean
sudo make TARGET=nrf52840 BOARD=dk DAO_INDUCTION_CLIENT=1 DAO_ATTACK_PERIOD_SECONDS=10 hello-world.upload
```

### Normal Node (Filtered DIO)

``` bash
sudo make TARGET=nrf52840 BOARD=dk distclean
sudo make TARGET=nrf52840 BOARD=dk FILTER_BR_DIO=1 hello-world.upload
```

------------------------------------------------------------------------

## 4. Border Router Networking (tunslip)

``` bash
make tunslip
sudo ./tunslip6 -s /dev/ttyACM0 fd00::1/64
```

> Replace `/dev/ttyACM0` with the correct device.

### Important:

-   Retrieve the local `fe80::` address
-   Add this address in `hello-world.c` to enforce the topology

------------------------------------------------------------------------

## 5. Device Monitoring (serialdump)

``` bash
make serialdump
sudo rlwrap ./serialdump -b115200 /dev/ttyACM4
```

> Replace `/dev/ttyACM4` with the correct device.

------------------------------------------------------------------------

## Notes

-   Always run `distclean` before flashing a different configuration
-   Ensure correct ACM ports for each device
-   Use multiple terminals for simultaneous monitoring
