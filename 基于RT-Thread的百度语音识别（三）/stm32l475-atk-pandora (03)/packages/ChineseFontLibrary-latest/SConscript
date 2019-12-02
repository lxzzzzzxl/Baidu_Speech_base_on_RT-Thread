from building import *

cwd  = GetCurrentDir()
path = [cwd + '/inc']

src  = Glob('src/*.c')


group = DefineGroup('Chinese_font_library', src, depend = ['PKG_USING_CHINESE_FONT_LIBRARY'], CPPPATH = path)

Return('group')



