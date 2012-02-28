all: \
    $(SHARED_INTERMEDIATE_DIR)/webkit/ColorData.cpp

$(SHARED_INTERMEDIATE_DIR)/webkit/ColorData.cpp \
    : \
    ../platform/ColorData.gperf \
    ../make-hash-tools.pl
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/webkit"
	perl ../make-hash-tools.pl "$(SHARED_INTERMEDIATE_DIR)/webkit" "../platform/ColorData.gperf"
