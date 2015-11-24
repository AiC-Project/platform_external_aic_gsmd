all:
	gcc -O2 -DFORTIFY_SOURCE=2 -fstack-protector simulator.c sim_card.c remote_call.c android_modem.c sysdeps_posix.c sms.c gsm.c config.c path.c -Wall -ggdb -Wextra
