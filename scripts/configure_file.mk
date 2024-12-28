DEFINITIONS := EXAMPLE_DEF=example_def

# Function to generate sed commands
define generate_sed_commands
$(foreach var, $(DEFINITIONS), -e 's|@$(var)@|$($(var))|g')
endef

%.h: %.in
	@echo "Configuring $< -> $@"
	@sed $(call generate_sed_commands) $< $@
