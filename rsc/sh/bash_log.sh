#!/bin/bash

# colors
RED="\e[31m"
GREEN="\e[32m"
YELLOW="\e[33m"
BLUE="\e[34m"
MAGENTA="\e[35m"
CYAN="\e[36m"
GRAY="\e[90m"
L_GRAY="\e[37m"
L_RED="\e[91m"
L_GREEN="\e[92m"
L_YELLOW="\e[93m"
L_BLUE="\e[94m"
L_MAGENTA="\e[95m"
L_CYAN="\e[96m"
WHITE="\e[97m"
RESET="\e[0m"

display_color_msg() {
	local color=$1
	local msg=$2
	echo -e "${color}${msg}${RESET}"
}

display_double_color_msg () {
	local color1=$1
	local msg1=$2
	local color2=$3
	local msg2=$4
	echo -e "${color1}${msg1}${RESET}${color2}${msg2}${RESET}"
}



# Colors for OK and KO
BOLD_GREEN="\e[1;32m"
BOLD_RED="\e[1;31m"


OK="[${BOLD_GREEN}OK${RESET}]"
KO="[${BOLD_RED}KO${RESET}]"


# D for log DEBUG
# I for log info
# W for log WARNING
# E for log ERROR
# N for log DEBUG no interpretation
log() {
	local level=${1}
	local msg="${2}"

	local date_str=${3}


	if [[ ! -z ${date_str} ]]; then
		date_str="${date_str} "
	fi

	case ${level} in

		"I")
			echo -e "${date_str}${GREEN}[ INF ]${RESET} ${msg}"
			;;

		"W")
			echo -e "${date_str}${YELLOW}[ WAR ]${RESET} ${msg}"
		;;

		"E")
			echo -e "${date_str}${RED}[ ERR ]${RESET} ${msg}"
		;;

		"D")
			echo -e "${date_str}${CYAN}[ DBG ]${RESET} ${msg}"
		;;

        "N")
            echo -ne "${date_str}${MAGENTA}[ DBG ]${RESET} "
            echo "${msg}"
        ;;

        "OK")
            echo -e "${date_str} ${OK} ${msg}"
        ;;

        "KO")
            echo -e "${date_str} ${KO} ${msg}"
        ;;

		*)
			echo -e "${date_str}${RED}Unkow level: ${msg} ${RESET}"
    	;;
	esac

}

log_date() {
	local level=${1}
	local msg="${2}"
	local date_str=$(date +"%Y/%m/%d %H:%M:%S")

	log ${level} "${msg}" ${MAGENTA}"[ ${date_str} ]"${RESET}
}

