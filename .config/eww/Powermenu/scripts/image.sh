FILE=$HOME/.face
if [[ -f "$FILE" ]]; then
    echo "../../../.face"
else
    echo "images/profile.png"
fi
