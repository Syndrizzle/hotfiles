if [[ $1 == "--image" ]]; then
    FILE=$HOME/.face
    if [[ -f "$FILE" ]]; then
        echo "../../../.face"
    else
        echo "images/profile.png"
    fi
fi

if [[ $1 == "--name" ]]; then
    getent passwd `whoami` | cut -d ":" -f 5 | cut -d "," -f 1 | tr -d "\n"
fi
