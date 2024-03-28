#!/bin/bash

NAME=$1_$(date +%s)
REGISTER=172.17.23.204:5000

ENVIRON=${HOME}/.environ
if [[ -f "${ENVIRON}" ]]; then
	echo "Load ${ENVIRON}"
	source "${ENVIRON}"
fi

if [[ ! "${DEBEMAIL}" || ! "${DEBFULLNAME}" ]]; then
	read -p "Please input your name (John Doe): " DEBFULLNAME
	read -p "Please input your email (johndoe@qnap.com): " DEBEMAIL
	
	read -p "Are you \"${DEBFULLNAME}\" and email is \"${DEBEMAIL}\" ? (y/N): " anwser
	
	case "${anwser}" in
		y|Y)
			cat > "${ENVIRON}" <<- __EOF__
			export DEBEMAIL="${DEBEMAIL}"
			export DEBFULLNAME="${DEBFULLNAME}"
			__EOF__
			
			echo "Save ${ENVIRON}"
		;;
	esac
fi

setup_shared_container()
{
	VOLUME="$1"
	VOLUME_SOURCE="${REGISTER}/${VOLUME}"
	VOLUME_CONTAINER="${VOLUME}_container"
	VOLUME_CONTAINER_ID=$(docker inspect --format="{{.Id}}" ${VOLUME_CONTAINER} 2> /dev/null)
	if [[ -z "${VOLUME_CONTAINER_ID}" ]]; then
		docker pull ${VOLUME_SOURCE}
		docker run -id --read-only=true --name ${VOLUME_CONTAINER} ${VOLUME_SOURCE} /bin/true
	fi
	echo -n "${VOLUME_CONTAINER}"
}

case $1 in
	x86_64)
		IMAGE=(
			--volumes-from=$(setup_shared_container ct_volume):ro
			-i ${REGISTER}/pure_builder
		)
		;;
	arm_64)
		IMAGE=(
			--volumes-from=$(setup_shared_container arm64_toolchain):ro
			-i ${REGISTER}/pure_builder
		)
		;;
	arm_al)
		IMAGE=(
			-i ${REGISTER}/builder
		)
		;;
	*)
		echo "$0 {x86_64|arm_64|arm_al} {other docker options ...}"
		exit 1
		;;
esac

BASE=(
	-t
	--rm
	--privileged
	-e DEBEMAIL=\"${DEBEMAIL}\"
	-e DEBFULLNAME=\"${DEBFULLNAME}\"
	-e TZ=Asia/Taipei
	-e LANG=en_US.UTF-8
	-u ${USER}
	-w ${HOME}
	-v ${HOME}:${HOME}
	-v /mnt:/mnt:ro
	-v /etc/passwd:/etc/passwd:ro
	--name=${NAME}
	--hostname=${NAME}
)

shift
docker run "${BASE[@]}" "${@}" "${IMAGE[@]}" /bin/bash