case $1 in
    "*") command="" ;;
    "up") command="--allow-boost -i 5" ;;
    "down") command="--allow-boost -d 5" ;;
    "toggle") command="-t" ;;
    [0-9]*)
        int=$(printf '%.0f' $1)
        command="--set-volume $int"
        ;;
esac

[ -n "$command" ] && pamixer $command

