CC   := clang
STD  := -std=gnu17

WARN := -Weverything \
        -Wno-padded -Wno-declaration-after-statement \
        -Wno-unsafe-buffer-usage -Wno-pre-c11-compat \
        -Wno-covered-switch-default -Wno-disabled-macro-expansion

debugFlags := $(STD) $(WARN) -Werror -g3 -O1 -fno-omit-frame-pointer \
               -fsanitize=address,undefined,float-divide-by-zero,implicit-conversion \
               -fno-sanitize-recover=all

releaseFlags  := $(STD) $(WARN) -O2 -g -DNDEBUG -flto=thin \
                  -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 \
                  -fstack-protector-strong -fstack-clash-protection \
                  -fcf-protection=full -ftrivial-auto-var-init=zero \
                  -fno-omit-frame-pointer -fPIE
releaseLDflags := -pie -fuse-ld=lld -flto=thin \
                   -Wl,-z,relro,-z,now -Wl,-z,noexecstack

debugPath = ./build/debug
releasePath = ./build/release

FILES = main.c interface.c
exeName = int

debug:$(FILES)
	mkdir -p $(debugPath)
	$(CC) $(debugFlags) $(FILES) -o $(debugPath)/$(exeName)

release:$(FILES)
	mkdir -p $(releasePath)
	$(CC) $(releaseFlags) $(releaseLDflags) $(FILES) -o $(releasePath)/$(exeName)
