1. modify  ： buildroot/configs/mesonaxg_s420_32_release_defconfig

-BR2_PACKAGE_WIFI_FW_WIFI_MODULE="bcm4356 AP6398"
+BR2_PACKAGE_WIFI_FW_WIFI_MODULE="qca9377"
 #BR2_PACKAGE_RTK8188EU=y
 #BR2_PACKAGE_RTK8188EU_LOCAL=y
 #BR2_PACKAGE_RTK8188EU_STANDALONE=y
@@ -421,13 +421,16 @@ BR2_PACKAGE_WIFI_FW_WIFI_MODULE="bcm4356 AP6398"
 #BR2_PACKAGE_REALTEK_BT_LOCAL_PATH="$(TOPDIR)/../hardware/aml-4.9/realtek/bluetooth"
 #BR2_PACKAGE_REALTEK_UART_BT=y
 #BR2_PACKAGE_REALTEK_USB_BT=y
-BR2_PACKAGE_BRCMAP6XXX=y
-BR2_PACKAGE_BRCMAP6XXX_LOCAL=y
-BR2_PACKAGE_BRCMAP6XXX_STANDALONE=y
-BR2_PACKAGE_BRCMAP6XXX_GIT_VERSION="m-amlogic"
-BR2_PACKAGE_BRCMAP6XXX_LOCAL_PATH="$(TOPDIR)/../hardware/aml-4.9/wifi/broadcom/drivers/ap6xxx"
-BR2_PACKAGE_BRCMAP6XXX_SDIO_VERSION="bcmdhd.1.363.59.144.x.cn"
-BR2_PACKAGE_BRCMAP6XXX_USB_VERSION="bcmdhd-usb.1.363.110.17.x"
+#BR2_PACKAGE_BRCMAP6XXX=y
+#BR2_PACKAGE_BRCMAP6XXX_LOCAL=y
+#BR2_PACKAGE_BRCMAP6XXX_STANDALONE=y
+#BR2_PACKAGE_BRCMAP6XXX_GIT_VERSION="m-amlogic"
+#BR2_PACKAGE_BRCMAP6XXX_LOCAL_PATH="$(TOPDIR)/../hardware/aml-4.9/wifi/broadcom/drivers/ap6xxx"
+#BR2_PACKAGE_BRCMAP6XXX_SDIO_VERSION="bcmdhd.1.363.59.144.x.cn"
+#BR2_PACKAGE_BRCMAP6XXX_USB_VERSION="bcmdhd-usb.1.363.110.17.x"
+BR2_PACKAGE_QUALCOMM_WIFI=y
+BR2_PACKAGE_QUALCOMM_QCA9377=y
+BR2_PACKAGE_QUALCOMM_WIFI_LOCAL_PATH="$(TOPDIR)/../hardware/aml-4.9/wifi/qualcomm/drivers"





2. add ： hardware/aml-4.9/wifi/qualcomm