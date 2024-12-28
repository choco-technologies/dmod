DEFINITIONS = $(filter DMOD%,$(.VARIABLES)) $(filter dmod%,$(.VARIABLES))

# Function to generate sed commands
define generate_sed_commands
$(foreach var, $(DEFINITIONS), -e 's|@$(var)@|$($(var))|g')
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
	@echo "Configuring file $<"
	@echo "Generating file $@"
	@echo "List of definitions: $(.VARIABLES)"
	@sed $(call generate_sed_commands) $< > $@
endef