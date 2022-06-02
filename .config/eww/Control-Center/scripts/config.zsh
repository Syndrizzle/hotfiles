function _set_vars() {
  typeset -gx DUNST_CACHE_DIR="$HOME/.cache/dunst"
  typeset -gx DUNST_LOG="$DUNST_CACHE_DIR/notifications.txt"
  
  typeset -gx DEFAULT_QUOTE="To fake it is to stand guard over emptiness. ── Arthur Herzog"
  typeset -gx DUNST_QUOTES="$DUNST_CACHE_DIR/quotes.txt"
}

function _unset_vars() {
  unset DUNST_CACHE_DIR
  unset DUNST_LOG
  unset DUNST_QUOTES
  unset DEFAULT_QUOTE
}
