TARGET = $(OFS_ASP_ROOT):ofs_iseries-dk

MHZ = 200MHz

EARLY = -fsycl-link=early 

SEED = 0

FLAGS = -fsycl -fPIC -fintelfpga -Xstarget=$(TARGET) -shared -Xshardware -Xsclock=$(MHZ) -Xsseed=$(SEED)

COMMON = -I ./include

s0:
	icpx $(FLAGS) $(COMMON) slot0/fourslot_iopipes_stream.cpp -o slot0/fourslot_iopipes_stream.so
s1:
	icpx $(FLAGS) $(COMMON) slot1/fourslot_iopipes_stream.cpp -o slot1/fourslot_iopipes_stream.so
s2:
	icpx $(FLAGS) $(COMMON) slot2/fourslot_iopipes_sink.cpp -o slot2/fourslot_iopipes_sink.so
s3:
	icpx $(FLAGS) $(COMMON) slot3/fourslot_iopipes_src.cpp    -o slot3/fourslot_iopipes_src.so

s0_early:
	icpx $(FLAGS) $(EARLY) $(COMMON) slot0/fourslot_iopipes_stream.cpp -o slot0/fourslot_iopipes_stream_early.so
s1_early:
	icpx $(FLAGS) $(EARLY) $(COMMON) slot1/fourslot_iopipes_stream.cpp -o slot1/fourslot_iopipes_stream_early.so
s2_early:
	icpx $(FLAGS) $(EARLY) $(COMMON) slot2/fourslot_iopipes_sink.cpp -o slot2/fourslot_iopipes_sink_early.so
s3_early:
	icpx $(FLAGS) $(EARLY) $(COMMON) slot3/fourslot_iopipes_src.cpp    -o slot3/fourslot_iopipes_src_early.so

all_early:
	make s0_early
	make s1_early
	make s2_early
	make s3_early

main: main.cpp
	icpx -fsycl -o main main.cpp

clean: 
	rm -rf slot0/*.so*
	rm -rf slot1/*.so*
	rm -rf slot2/*.so*
	rm -rf slot3/*.so*