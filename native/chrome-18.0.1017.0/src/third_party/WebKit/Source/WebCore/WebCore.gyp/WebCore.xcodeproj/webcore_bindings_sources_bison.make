all: \
    $(SHARED_INTERMEDIATE_DIR)/webkit/CSSGrammar.cpp \
    $(SHARED_INTERMEDIATE_DIR)/webkit/XPathGrammar.cpp

$(SHARED_INTERMEDIATE_DIR)/webkit/CSSGrammar.cpp \
    $(SHARED_INTERMEDIATE_DIR)/webkit/CSSGrammar.h \
    : \
    ../css/CSSGrammar.y
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/webkit"
	python scripts/rule_bison.py "../css/CSSGrammar.y" "$(SHARED_INTERMEDIATE_DIR)/webkit"

$(SHARED_INTERMEDIATE_DIR)/webkit/XPathGrammar.cpp \
    $(SHARED_INTERMEDIATE_DIR)/webkit/XPathGrammar.h \
    : \
    ../xml/XPathGrammar.y
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/webkit"
	python scripts/rule_bison.py "../xml/XPathGrammar.y" "$(SHARED_INTERMEDIATE_DIR)/webkit"
