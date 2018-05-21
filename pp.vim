let g:pp='/usr/local/bin/pp'
map \p :Callpp 
map \t :CallppTab 
fu! _pp()
	let shellredir=&shellredir
	set shellredir=2>%s
	exe "normal :new:0r!" . g:pp . " -2 . :let a=getline('.'):q!"
	let &shellredir=shellredir
	return a
endf
command! Callpp exe 'e ' . _pp()
command! CallppTab exe 'tabnew ' . _pp()
