UPDATE_FILE=updater.sh.gpg
UPDATE_SCRIPT=updater.sh
VERSION_FILE=newestVersion.txt

UPDATE_SERVER=http://projectnova.org/updates

rm -f $UPDATE_FILE
rm -f $UPDATE_SCRIPT
rm -f $VERSION_FILE

# return 0 if program version is equal or greater than check version
check_version()
{
    local version=$1 check=$2
    local winner=$(echo -e "$version\n$check" | sed '/^$/d' | sort -nr | head -1)
    [[ "$winner" = "$version" ]] && return 0
    return 1
}

echo "############################ Checking if new updates available ############################"
wget -nv $UPDATE_SERVER/$VERSION_FILE

if [ $? -ne 0 ]
then
	echo "ERROR: unable to fetch update information from server"
	exit 1
fi

if check_version $(cat ~/.config/nova/config/version.txt) $(cat $VERSION_FILE); then
	echo "#### Your version of nova is currently at: \"$(cat ~/.config/nova/config/version.txt)\", but the newest version on update server is \"$(cat $VERSION_FILE)\" ####"
	echo "#### No updates are currently available ####"

	exit 1
else
	echo "#### Your version of nova is currently at : \"$(cat ~/.config/nova/config/version.txt)\" and can be upgraded to \"$(cat $VERSION_FILE)\" ####"
fi


echo "############################ Stopping Novad and Haystack ############################"
stop haystack
stop nova
novacli stop nova
novacli stop haystack

echo "############################ Downloading Update File ############################"
wget -nv $UPDATE_SERVER/$UPDATE_FILE


echo "############################ Checking Update File Integrity ############################"
gpg --batch --no-default-keyring --keyring "/usr/share/nova/sharedFiles/datasoftkeys.gpg" --verify $UPDATE_FILE
if [ $? -ne 0 ]
then
	echo "ERROR: signature of update file can not be verified. Cancelling."
	exit 1
fi

echo "############################ Decrypting Update File ############################"
gpg --batch --no-default-keyring --keyring "/usr/share/nova/sharedFiles/datasoftkeys.gpg" --output $UPDATE_SCRIPT --decrypt $UPDATE_FILE
if [ $? -ne 0 ]
then
	echo "ERROR: update file can not be decrypted. Cancelling."
	exit 1
fi


/bin/bash $UPDATE_SCRIPT

