___DEFS = $(filter DMOD%,$(.VARIABLES)) $(filter dmod%,$(.VARIABLES))

# Function to generate sed commands
define generate_sed_commands
$(foreach var, $(___DEFS), -e 's|@$(var)@|$($(var))|g')
endef

#
#	Configuration of file
#
# 		$1 - source file
# 		$2 - destination file
#
define configure_file
	@echo "Removal of file $2"
	@$(RM) -f $2
	@echo "Configuring file $1"
	@echo "Generating file $2"
	@sed $(call generate_sed_commands) $1 > $2
endef

#
#	Generates a rule to generate a header file
#
define generate_header_rule
$(2): $(1) 
	@echo "Generating $2 from $1..."
	@$(call configure_file,$1,$2)
endef

#
#   Touches header
#
define touch_header
	@if [ "$(DMOD_CACHE_CHANGED)" = "ON" ]; then \
		echo "Touching $1..."; \
		touch $1; \
	else \
		echo "Cache did not change. Skipping update of $1"; \
	fi
endef

#
#   Touches all headers
#
define touch_headers
$(foreach pair,
	$1,
	$(call touch_header,$(word 1,$(subst =, ,$(pair))))
)
endef

#
#	Generates a rule to generate a header file
#
define generate_headers_rules
$(foreach pair,
	$1,
	$(eval 
		$(call generate_header_rule,$(word 1,$(subst =, ,$(pair))),$(word 2,$(subst =, ,$(pair))))
	)
)
endef

#
#	Generates a rules for every object file
#
define generate_cobject_rule
$2: $1 
	@echo "Compiling $1 to $2..."
	$(CC) $(CFLAGS) -c $1 -o $2
	@echo "Listing generation for $2..."
	$(CC) $(CFLAGS) -g -S -fverbose-asm -c $1 -o $(basename $2).lst
endef

#
#	Generates a rules for every object file
#
define generate_cxxobject_rule
$2: $1 
	@echo "Compiling $1 to $2..."
	$(CXX) $(CXXFLAGS) -c $1 -o $2
	@echo "Listing generation for $2..."
	$(OBJDUMP) -d $2 > $(basename $2).lst
endef

#
#	Generates a rules for every object file
#
define generate_cobjects_rule
$(foreach pair
	,$(DMOD_COBJECTS)
	,$(eval 
		$(call generate_cobject_rule,$(word 1,$(subst =, ,$(pair))),$(word 2,$(subst =, ,$(pair))))
	)
)
endef

#
#	Generates a rules for every object file
#
define generate_cxxobjects_rule
$(foreach pair
	,$(DMOD_CXXOBJECTS)
	,$(eval 
		$(call generate_cxxobject_rule,$(word 1,$(subst =, ,$(pair))),$(word 2,$(subst =, ,$(pair))))
	)
)
endef