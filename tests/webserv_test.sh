#!/bin/bash

#Path to the executable
exec="../webserv"

#Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
PINK='\033[35m'
BGPINK='\033[45m'
BOLD='\033[1m'
RES='\033[0m'

#variables for address and other usage
httpPrefix="http://"
serverAddr="127.0.0.2"
defPort="8888"
serv1Port="9999"
testFile="Testfile with spaces.txt"
testFileEncoded="${testFile// /%20}"
dir="upload/tmp"

#Menu variables
MENU_ITEMS=(
	"Run config tests"
	"Run curl tests on default server (port $defPort)"
	"Run curl tests on server1 (port $serv1Port)"
	"Run netcat tests on default server (port $defPort)"
	"Run netcat tests on server1 (port $serv1Port)"
	"Run a siege on our server"
	"Run all tests (except siege)"
	"Exit"
)

#file reading for netcat tests
boundary="----WebKitFormBoundary42"
filename="$testFile"
pre="------${boundary}\r\nContent-Disposition: form-data; name=\"file\"; filename=\"${filename}\"\r\nContent-Type: text/plain\r\n\r\n"
post="\r\n------${boundary}--\r\n"
body="$(printf "%b" "$pre"; cat "$testFile"; printf "%b" "$post")"
content_length=$(printf "%s" "$body" | wc -c)
request_valid="POST /upload HTTP/1.1\r\nHost: ${serverAddr}:__PORT__\r\nContent-Type: multipart/form-data; boundary=${boundary}\r\nContent-Length: ${content_length}\r\n\r\n${body}"
request_invalid="POST / HTTP/1.1\r\nHost: ${serverAddr}:__PORT__\r\nContent-Type: multipart/form-data; boundary=${boundary}\r\nContent-Length: ${content_length}\r\n\r\n${body}"

#Import tests from file
source ./curl_tests.sh
source ./nc_tests.sh
source ./config_tests.sh

#Function to compile the program if it is not found
compile_program() {
	if [ ! -x "$exec" ]; then
		cd .. && make
		cd tests
		if [ ! -x "$exec" ]; then
			echo "$exec not found, please build manually"
			exit 1
		fi
	fi
}

#Function to display headers
display_header() {
	local title="$1"
	local title_length=${#title}
	local box_width=54
	local padding=$(( (box_width - title_length - 2) / 2 ))
	local left_pad=$(printf "%*s" $padding)
	local right_pad=$(printf "%*s" $(( box_width - title_length - padding - 2 )))
	local term_width=$(tput cols)
	local box_padding=$(( (term_width - box_width) / 2 ))
	local box_pad=$(printf "%*s" $box_padding)
	clear
	tput civis
	echo -e "${box_pad}${GREEN}${BOLD}╔════════════════════════════════════════════════════╗${RES}"
	echo -e "${box_pad}${GREEN}${BOLD}║${left_pad}${title}${right_pad}║${RES}"
	echo -e "${box_pad}${GREEN}${BOLD}╚════════════════════════════════════════════════════╝${RES}\n"
}

#Function to display the menu
display_selection() {
	for i in "${!items[@]}"; do
		if [[ $i -eq selected ]]; then
			echo -e "${PINK}${BOLD}►${RES} ${BGPINK}${items[$i]}${RES}"
		else
			echo -e "  ${PINK}${items[$i]}${RES}"
		fi
	done
	echo -e "${GREEN} ↑/↓\t: Navigate\tESC, Q or q\t: Exit${RES}"
	echo -e "${GREEN} Enter\t: Confirm${RES}"
}

#Function to handle menu interactions
handle_selection() {
	local header_title="$1"
	shift
	local items=("$@")
	local selected=0

	display_header "$header_title"
	display_selection
	while true; do
		IFS= read -rsn1 key
		case "$key" in
			$'\x1b')
				IFS= read -rsn1 -t 0.1 key2
				if [[ -z "$key2" ]]; then
					return 255
				else
					# Arrow key sequence
					case "$key2" in
						'[')
							IFS= read -rsn1 key3
							case "$key3" in
								'A')  # Up arrow
									((selected--))
									if [[ $selected -lt 0 ]]; then
										selected=$((${#items[@]} - 1))
									fi
									display_header "$header_title"
									display_selection
									;;
								'B')  # Down arrow
									((selected++))
									if [[ $selected -ge ${#items[@]} ]]; then
										selected=0
									fi
									display_header "$header_title"
									display_selection
									;;
							esac
							;;
					esac
				fi
				;;
			'')  # Enter key
				return $selected
				;;
			q|Q)  # Q key
				return 255
				;;
		esac
	done
}

#Function to clean up test directories
cleanup_dirs() {
	if [[ -d ../www/default/${dir} ]]; then
		rmdir ../www/default/${dir} 2>/dev/null
	fi
	if [[ -d ../www/server1/${dir} ]]; then
		rmdir ../www/server1/${dir} 2>/dev/null
	fi
}

#Function for asking if make fclean should be run
make_fclean() {
	if [[ -f "${testFile}" ]]; then
		rm "${testFile}"
	fi
	cleanup_dirs
	echo -n "Run make fclean? [Y/N]"
	read answer
	if [[ "$answer" =~ ^[Yy]$ ]]; then
		cd .. && make fclean
		cd tests
	fi
}

#Function to skip ahead or go back to the menu
skip_or_exit() {
	echo -e "\n${YELLOW}Press Enter to continue with the next test or ESC to return to the menu...${RES}"
	IFS= read -rsn1 key
	if [[ $key == $'\e' ]]; then
		clear
		return 1
	elif [[ "$key" == "" ]]; then
		return 0
	fi
}

#Testing all invalid configuration files
run_config_tests() {
	local return_prompt=$1

	display_header "INVALID CONFIGURATIONS"
	for file in "${files[@]}"; do
		echo -e "${PINK}\nProcessing file:${RES} ${file}\n$"
		cat "${filePath}${file}${fileExt}"
		echo -e "\n\n${PINK}${exec} output:\n\n${RES}"
		"$exec" "${filePath}${file}${fileExt}"
		skip_or_exit;
		if [[ $? -eq 1 ]]; then
			return 1
		fi
		clear
	done
	echo -e "\n${GREEN}Config tests completed!${RES}"
	if [[ "$return_prompt" == "true" ]]; then
		echo -e "${YELLOW}Press Enter to return to menu...${RES}"
		read
	else
		echo -e "${YELLOW}Continuing to next test suite...${RES}"
		sleep 2
	fi
	return 0
}

run_siege_tests() {
	local server_items=("Default Server (port $defPort)" "Server1 (port $serv1Port)" "Return to Main Menu")
	local target_items=("Index" "Empty page" "Other" "Return to Main Menu")

	#Server selection
	handle_selection "SIEGE TEST - SELECT SERVER" "${server_items[@]}"
	local server_choice=$?
	if [[ $server_choice -eq 255 ]]; then
		return 1
	fi
	case $server_choice in
		0) local selected_port=$defPort ;;
		1) local selected_port=$serv1Port ;;
		2) return 1 ;;
	esac
	#Duration input
	display_header "SIEGE TEST - INPUT DURATION"
	echo -e "${YELLOW}Please input the duration of the test run in seconds (default 60):${RES}\n"
	read duration
	if [[ -z "$duration" ]]; then
		duration=60
	fi
	#Concurrent users input
	display_header "SIEGE TEST - INPUT USERS"
	echo -e "${YELLOW}Please input the number of concurrent users (default 25, max 255):${RES}\n"
	read users
	if [[ -z "$users" ]]; then
		users=25
	fi
	#Target selection
	handle_selection "SIEGE TEST - SELECT TARGET" "${target_items[@]}"
	local target_choice=$?
	if [[ $target_choice -eq 255 ]]; then
		return 1
	fi
	case $target_choice in
		0) local selected_target="index.html" ;;
		1) local selected_target="./html/empty_file.html" ;;
		2)
			display_header "SIEGE TEST - CUSTOM TARGET"
			echo -e "${YELLOW}Please input the custom target path WITHOUT leading /:${RES}\n"
			read selected_target
			if [[ -z "$selected_target" ]]; then
				selected_target="index.html"
			fi
			;;
		3) return 1 ;;
	esac
	#Run siege test
	display_header "RUNNING SIEGE TEST"
	echo -e "${PINK}Server:${RES}\t${serverAddr}:${selected_port}\n"
	echo -e "${PINK}Target:${RES}\t${selected_target}"
	echo -e "${PINK}Duration:${RES}\t${duration} seconds\n"
	echo -e "${GREEN}${BOLD}Starting siege... (Press Ctrl+C to abort)${RES}\n"
	echo -e "${GREEN}${BOLD}═══════════════════════════════════════════════════${RES}\n"
	tput cnorm
	siege -b -c${users} -t${duration}S ${httpPrefix}${serverAddr}:${selected_port}/${selected_target}
	if [[ $? -ne 0 ]]; then
		echo -e "${RED}Siege command failed${RES}"
	fi
	tput civis
	echo -e "\n${GREEN}Siege test completed!${RES}"
	echo -e "${YELLOW}Press Enter to return to menu...${RES}"
	read
	return 0
}

#Testing function
run_tests() {
	local port=$1
	local server_name=$2
	local tool=$3
	local return_prompt=${4:-true}

	display_header "${tool^^} TESTS ON ${server_name} (Port ${port})"
	echo -e "${RED}${BOLD}\nMake sure webserver is running with the correct config for ${server_name} on port ${port}${RES}\n"
	echo -e "${YELLOW}\nPress Enter to continue or escape to return to menu...${RES}"
	IFS= read -rsn1 key
	if [[ $key == $'\e' ]]; then
		return 1
	fi
	if [[ "${port}" == "$defPort" ]]; then
		mkdir -p ../www/default/${dir}
	fi
	if [[ "${port}" == "$serv1Port" ]]; then
		mkdir -p ../www/server1/${dir}
	fi
	if [[ "$tool" == "curl" ]]; then
		tests=("${curl_tests[@]}")
		titles=("${curl_titles[@]}")
	elif [[ "$tool" == "nc" ]]; then
		tests=("${netcat_tests[@]}")
		titles=("${netcat_titles[@]}")
	fi
	for ((i=0; i<${#tests[@]}; ++i)); do
		display_header "${tool^^} TESTS ON ${server_name} (Port ${port})"
		command="${tests[$i]}"
		command="${command//__PORT__/$port}"
		title="${titles[$i]}"
		echo -e "${PINK}\n\nCurrently running: ${RES}${title} ${GREEN}${tool} ${command}\n\n${RES}"
		if [[ "$tool" == "curl" ]]; then
			eval curl $command
		else
			printf "%b" "$command" | nc ${serverAddr} $port
		fi
		skip_or_exit
		if [[ $? -eq 1 ]]; then
			cleanup_dirs
			return 1
		fi
	done
	cleanup_dirs
	echo -e "\n${GREEN}${tool^^} tests completed!${RES}\n"
	if [[ "$return_prompt" == "true" ]]; then
		echo -e "${YELLOW}Press Enter to return to menu...${RES}"
		read
	else
		echo -e "${YELLOW}Continuing to next test suite...${RES}"
		sleep 2
	fi
	return 0
}

#Main function
main() {
	compile_program
	trap 'tput cnorm; clear' EXIT INT TERM
	echo "Testfile for webserv testing" > "${testFile}"
	while true; do
		handle_selection "WEBSERV TEST SUITE" "${MENU_ITEMS[@]}"
		choice=$?
		if [[ $choice -eq 255 ]]; then
			clear
			rm "${testFile}"
			make_fclean
			clear
			echo -e "${GREEN}Goodbye, thanks for testing!${RES}"
			tput cnorm
			exit 0
		fi
		case $choice in
		 0) #Run the config tests
		 	run_config_tests "true"
			;;
		1) #Run curl tests on the default server
			run_tests "$defPort" "DEFAULT SERVER" "curl" "true"
			;;
		2) #Run curl tests on server1
			run_tests "$serv1Port" "SERVER1" "curl" "true"
			;;
		3) #Run nc tests on the default server
			run_tests "$defPort" "DEFAULT SERVER" "nc" "true"
			;;
		4) #Run nc tests on server1
			run_tests "$serv1Port" "SERVER1" "nc" "true"
			;;
		5) #Run siege tests
			run_siege_tests
			;;
		6) #Run all tests
			run_config_tests "false"
			if [[ $? -eq 1 ]]; then
				continue
			fi
			run_tests "$defPort" "DEFAULT SERVER" "curl" "false"
			if [[ $? -eq 1 ]]; then
				continue
			fi
			run_tests "$serv1Port" "SERVER1" "curl" "false"
			if [[ $? -eq 1 ]]; then
				continue
			fi
			run_tests "$defPort" "DEFAULT SERVER" "nc" "false"
			if [[ $? -eq 1 ]]; then
				continue
			fi
			run_tests "$serv1Port" "SERVER1" "nc" "false"
			if [[ $? -eq 0 ]]; then
				echo -e "\n${GREEN}${BOLD}All tests completed!${RES}\n"
				echo -e "${YELLOW}Press Enter to return to menu...${RES}"
				read
			fi
			;;
		7) #Exit
			clear
			rm "${testFile}"
			make_fclean
			clear
			echo -e "${GREEN}Goodbye, thanks for testing!${RES}"
			tput cnorm
			exit 0
			;;
		esac
	done
}

main
