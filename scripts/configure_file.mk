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
	@echo "Configuring file $<"
	@sed $(call generate_sed_commands) $< > $@
endef