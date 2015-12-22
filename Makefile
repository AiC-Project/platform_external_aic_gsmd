CC=g++
SOURCES=sensors_packet.pb.cc simulator.c sim_card.c remote_call.c android_modem.c sysdeps_posix.c sms.c gsm.c config.c path.c
CFLAGS=-O2 -fstack-protector -DFORTIFY_SOURCE=2 -DHOST_BUILD -ggdb -Wall
SCANEXTRA=-enable-checker alpha.core.BoolAssignment -enable-checker alpha.core.CallAndMessageUnInitRefArg -enable-checker alpha.core.CastSize -enable-checker alpha.core.CastToStruct -enable-checker alpha.core.FixedAddr -enable-checker alpha.core.IdenticalExpr -enable-checker alpha.core.PointerArithm -enable-checker alpha.core.PointerSub -enable-checker alpha.core.SizeofPtr -enable-checker alpha.core.TestAfterDivZero -enable-checker alpha.deadcode.UnreachableCode -enable-checker alpha.security.ArrayBound -enable-checker alpha.security.ArrayBoundV2 -enable-checker alpha.security.MallocOverflow -enable-checker alpha.security.ReturnPtrRange -enable-checker alpha.unix.MallocWithAnnotations -enable-checker alpha.unix.SimpleStream -enable-checker alpha.unix.Stream -enable-checker alpha.unix.cstring.NotNullTerminated
all:
	pkg-config --cflags protobuf
	$(CC) $(CFLAGS) $(SOURCES) `pkg-config --cflags --libs protobuf`
scan:
	scan-build $(CC) $(CFLAGS) $(SOURCES) `pkg-config --cflags --libs protobuf`

scan_all:
	scan-build $(SCANEXTRA) $(CC) $(CFLAGS) $(SOURCES) `pkg-config --cflags --libs protobuf`

update:
	cp -rvf *.c *.h ~/aic/work/vm/device/aicVM/goby/gsmd/
