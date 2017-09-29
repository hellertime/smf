#!/bin/bash

# Copyright 2017 Alexander Gallego
#
set -ex

function die_unsupported {
    local system="$1"
    echo "Unsupported system: $system" && exit 1
}

function install_brew {
    local system="$1"
    if [[ -z $(which brew) ]]; then
        case $system in
            darwin)
                /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
                ;;
            *)
                # skip brew on this platform
                ;;
        esac
    fi
}

function install_vagrant {
    local system="$1"
    if [[ -z $(which vagrant) ]]; then
        case $system in
            fedora)
                sudo dnf install vagrant vagrant-libvirt vagrant-libvirt-doc vagrant-lxc
                ;;
            ubuntu)
                sudo apt-get install vagrant
                ;;
            darwin)
                brew cask install vagrant
                ;;
            *)
                die_unsupported $system
                ;;
        esac
    fi
}

function install_virtualbox {
    local system="$1"
    if [[ -z $(which VirtualBox) ]]; then
        case $system in
            fedora)
                pushd /etc/yum.repos.d/
                ## Fedora 25/24/23/22/21/20/19/18/17/16 users
                sudo bash -c 'wget http://download.virtualbox.org/virtualbox/rpm/fedora/virtualbox.repo | tee '
                popd
                sudo dnf update
                sudo dnf install binutils gcc make patch libgomp glibc-headers glibc-devel \
                     kernel-headers kernel-PAE-devel dkms libffi-devel ruby-devel VirtualBox
                ;;
            ubuntu)
                sudo apt-get install virtualbox
                ;;
            darwin)
                brew cask install virtualbox
                ;;
            *)
                die_unsupported $system
                ;;
        esac
    fi
}

echo "usage: provision_vagrant.sh (debug|quiet)?"
if [[ $1 == "debug" ]]; then
    export VAGRANT_LOG=info
fi

export PYTHONUNBUFFERED=1

system="darwin"
if [[ $(which lsb_release) != "" ]]; then
    system=$(lsb_release -si | tr '[:upper:]' '[:lower:]' )
fi

git_root=$(git rev-parse --show-toplevel)
cd $git_root

if [[ system == "darwin" ]]; then
    if [[ $(which brew) == "" ]]; then
        /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi
fi

install_brew $system
install_virtualbox $system
install_vagrant $system

if [[ $(vagrant plugin list | grep vbguest) == "" ]]; then
    vagrant plugin install vagrant-vbguest
fi
vagrant halt && vagrant up --provision --provider virtualbox
[[ $? != 0 ]] && echo "Broken Vagrant" && exit $?
