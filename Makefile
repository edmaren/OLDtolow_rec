PS5_HOST ?= ps5
PS5_PORT ?= 9021

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
$(error PS5_PAYLOAD_SDK is undefined)
endif

ELF    := tolow_rec.elf
CFLAGS := -std=c++11 -Wall -Werror -g

all: $(ELF)

$(ELF): source/main.cpp
	$(CXX) $(CFLAGS) -o $@ $^

clean:
	rm -f $(ELF)
