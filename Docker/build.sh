#!/bin/bash
#
#       Builds an image required for building of the ChocoOS system
#

#
#   Path to the directory with this script
#
THIS_SCRIPT_PATH="${BASH_SOURCE[0]}"
THIS_DIR="$( cd "$( dirname "$THIS_SCRIPT_PATH" )" >/dev/null && pwd )"
ROOT_DIR="$THIS_DIR/.."
DOCKERFILE_PATH="$THIS_DIR/Dockerfile"

#
#   Path to the configuration file
#
CONFIGURATION_FILE_PATH=~/.choco-scripts.cfg
SCRIPT_DESCRIPTION=""

#
#   Installs choco-scripts
#
function installChocoScripts()
{
    echo "Installation of the choco-scripts"

    # This line installs wget tool - you don't need to use it if you already have it
    apt-get update && apt-get install -y wget

    # This downloads an installation script and run it 
    wget -O - https://release.choco-technologies.com/scripts/install-choco-scripts.sh | bash
}

#
#   Verification of the choco scripts installation
#
if [ -f "$CONFIGURATION_FILE_PATH" ]
then 
    source $CONFIGURATION_FILE_PATH
else 
    printf "\033[31;1mChoco-Scripts are not installed for this user\033[0m\n\n"
    printf "      \033[37;1mYou can find the installation instruction here: \033[0m\n"
    printf "            \033[34;1mhttps://bitbucket.org/chocotechnologies/scripts/src/master/\033[0m\n\n"

    while true
    do
        read -p "Do you want to try to auto-install it? [Y/n]: " answer
        case $answer in 
            [Yy]* ) installChocoScripts; break;;
            [Nn]* ) echo "Skipping installation"; exit 1;;
            * ) echo "Please answer Y or n";;
        esac
    done
    exit 1
fi

#
#   Information message
#
echo "Using choco-scripts from path $CHOCO_SCRIPTS_PATH in version $CHOCO_SCRIPTS_VERSION"

#
#   Importing of the framework main script
#
source $(getChocoScriptsPath)

#
#   The function prepares a framework script to work
#
function prepareScript()
{
    defineScript "$0" "Builds an image required for building of the ChocoOS system"
    
    addCommandLineOptionalArgument 'IMAGE_NAME' '--image-name' not_empty_string 'Name of the image to build' 'chocotechnologies/dmod' ''
    addCommandLineOptionalArgument 'IMAGE_VERSION' '--image-version' not_empty_string 'Version of the image to build' 'latest' ''

    
    disableConfigurationPrinting
    parseCommandLineArguments "$@"
}

#
#   Builds the image
#   
function build()
{
    local old_pwd=$(pwd)
    cd "$THIS_DIR/.."
    doCommandAsStepWithSpinner "Building the image $IMAGE_NAME:$IMAGE_VERSION" docker build --squash -t "$IMAGE_NAME:$IMAGE_VERSION" -f "$DOCKERFILE_PATH" "$ROOT_DIR"
    cd "$old_pwd"
}

#######################################################################################
#
#   MAIN
#
prepareScript "$@"
build 
