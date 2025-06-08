project('gtkapps', 'c', version: '0.0.1')

gtk = dependency('gtk4')
gio = dependency('gio-2.0')
giounix = dependency('gio-unix-2.0')
glib = dependency('glib-2.0')
gtk4layershell = dependency('gtk4-layer-shell-0')
cjson = dependency('libcjson')

deps = [gtk, gtk4layershell, cjson, gio, giounix, glib]

src = ['main.c']

executable('gtkapps', sources: src, dependencies: deps, install: true)
