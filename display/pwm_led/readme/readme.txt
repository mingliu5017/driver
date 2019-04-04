1、查看对应引脚功能
cat /sys/kernel/debug/pinctrl/pinctrl@ff634480/pinmux-pins
2、查看引脚功能定义，并修改dts
pinmux定义
kernel\aml-4.9\drivers\amlogic\pinctrl\pinctrl-meson-axg.c
static const char * const pwm_a_groups[] = {
	"pwm_a_z", "pwm_a_x18", "pwm_a_x20", "pwm_a_a",
};

static const char * const pwm_b_groups[] = {
	"pwm_b_z", "pwm_b_x", "pwm_b_a",
};

static const char * const pwm_c_groups[] = {
	"pwm_c_x10", "pwm_c_x17", "pwm_c_a",
};

static const char * const pwm_d_groups[] = {
	"pwm_d_x11", "pwm_d_x16",
};
static const char * const pwm_ao_a_groups[] = {
	"pwm_ao_a",
};

static const char * const pwm_ao_b_groups[] = {
	"pwm_ao_b_ao2", "pwm_ao_b_ao12",
};

static const char * const pwm_ao_c_groups[] = {
	"pwm_ao_c_ao8", "pwm_ao_c_ao13",
};

static const char * const pwm_ao_d_groups[] = {
	"pwm_ao_d",
};

3、pwm定义开启查找
kernel/aml-4.9/arch/arm64/boot/dts/amlogic/mesonaxg.dtsi
     pwm_ab: pwm@ffd1b000 {
         compatible = "amlogic,axg-ee-pwm";
         reg = <0x0 0xffd1b000  0x0 0x20>;
         #pwm-cells = <3>;
        clocks = <&xtal>,<&xtal>,<&xtal>,<&xtal>;
         clock-names = "clkin0","clkin1","clkin2","clkin3";
         /*default xtal 24M  clkin0-clkin2 and clkin1-clkin3
          *should be set the same
          */
         status = "disabled";
     };
     pwm_cd: pwm@ffd1a000 {
         compatible = "amlogic,axg-ee-pwm";
         reg = <0x0 0xffd1a000  0x0 0x20>;
         #pwm-cells = <3>;
        clocks = <&xtal>,<&xtal>,<&xtal>,<&xtal>;
        clock-names = "clkin0","clkin1","clkin2","clkin3";
        status = "disabled";
     };

4、pwm功能开启
 &pwm_ab {
     status = "okay";
 };
 
 &pwm_cd {
    status = "okay";
 };

5、开启pwm led 驱动dts
kernel/aml-4.9/arch/arm64/boot/dts/amlogic/axg_s420_v03.dts
	pwmleds {
		compatible = "pwm-leds";
		pinctrl-names = "default";
		pinctrl-0 = <&led_green_pins &led_red_pins &led_blue_pins>;
		status = "okay";
		green {
				label="green";
				pwms = <&pwm_cd MESON_PWM_1 100000 0>;
				max-brightness = <255>;
				linux,default-trigger = "none";
				brightness = <0>;
		};
		red {
				label="red";
				pwms = <&pwm_cd MESON_PWM_0 100000 0>;
				max-brightness = <255>;
				linux,default-trigger = "none";
				brightness = <0>;
		};
		blue {
				label="blue";
				pwms = <&pwm_ab MESON_PWM_1 100000 0>;
				max-brightness = <255>;
				linux,default-trigger = "none";
				brightness = <0>;
		};
	};


kernel/aml-4.9/arch/arm64/boot/dts/amlogic/mesonaxg.dtsi

    led_green_pins:led_green_pins {
        mux {
            groups ="pwm_d_x16";
            function = "pwm_d";
        };
    };

     led_red_pins:led_red_pins {
       mux {
            groups ="pwm_c_x17";
            function = "pwm_c";
         };
     };

     led_blue_pins:led_blue_pins {
         mux {
             groups ="pwm_b_x";
             function = "pwm_b";
         };
     };

6、驱动编译选项开启
CONFIG_AMLOGIC_PWM=y
CONFIG_PWM=y
CONFIG_LEDS_PWM=y