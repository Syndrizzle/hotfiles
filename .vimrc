call plug#begin()
Plug 'catppuccin/vim'
Plug 'tpope/vim-fugitive'
Plug 'tpope/vim-surround'
Plug 'scrooloose/nerdtree'
Plug 'scrooloose/syntastic'
Plug 'scrooloose/nerdcommenter'
Plug 'valloric/youcompleteme'
Plug 'ap/vim-css-color'
Plug 'hail2u/vim-css3-syntax'
Plug 'davidhalter/jedi-vim'
Plug 'hdima/python-syntax'
Plug 'sheerun/vim-polyglot'
Plug 'rrethy/vim-hexokinase', { 'do': 'make hexokinase' }
Plug 'preservim/nerdtree'
Plug 'yuttie/comfortable-motion.vim'
Plug 'baskerville/vim-sxhkdrc'
call plug#end()
colorscheme catppuccin_mocha
set termguicolors
nnoremap <leader>n :NERDTreeFocus<CR>
nnoremap <C-n> :NERDTree<CR>
nnoremap <C-t> :NERDTreeToggle<CR>
nnoremap <C-f> :NERDTreeFind<CR>
autocmd VimEnter * NERDTree
set mouse=a
let g:NERDTreeMouseMode=3
noremap <silent> <ScrollWheelDown> :call comfortable_motion#flick(40)<CR>
noremap <silent> <ScrollWheelUp>   :call comfortable_motion#flick(-40)<CR>
command W :execute ':silent w !sudo tee % > /dev/null' | :edit!
command! -nargs=0 Sw w !sudo tee % > /dev/null
autocmd bufenter * if (winnr("$") == 1 && exists("b:NERDTree") && b:NERDTree.isTabTree()) | q | endif
set ttymouse=sgr
