BUILD ?= obj

CFLAGS += -Iinclude

all: libhjbcrypt.so attack
.PHONY: clean

-include source/Makefile

hjb-obj = $(addprefix ${BUILD}/, ${hjb-obj-y})
attack-obj = $(addprefix ${BUILD}/, ${attack-obj-y})

deps += $(hjb-obj:.o=.d)
deps += $(attack-obj:.o=.d)

-include ${deps}

libhjbcrypt.so: CFLAGS += -fPIC
libhjbcrypt.so: LDFLAGS += -shared
libhjbcrypt.so: $(hjb-obj)
	@echo "LD $(notdir $@)"
	@mkdir -p "$(dir $@)"
	@${CC} ${hjb-obj} -o $@ ${LDFLAGS} ${CFLAGS} ${LIBS}

attack: libhjbcrypt.so ${attack-obj}
	@echo "LD $(notdir $@)"
	@mkdir -p "$(dir $@)"
	@${CC} ${attack-obj} -o $@ ${LDFLAGS} ${CFLAGS} ${LIBS} -L. -lhjbcrypt

$(BUILD)/%.o: %.c
	@echo "CC $<"
	@mkdir -p "$(dir $@)"
	@${CC} -c $< -o $@ ${CFLAGS} -MT $@ -MMD -MP -MF $(@:.o=.d)

clean:
	@rm -f attack libhjbcrypt.so
	@rm -rf ${BUILD}
