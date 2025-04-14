project('Air Monitor', 'cpp')

incdir = include_directories('/home/hachikuji/Documents/imgui/backends', '/home/hachikuji/Documents/imgui', '/home/hachikuji/Documents/imgui/implot')

glfw_lib = subproject('glfw').get_variable('glfw_dep')
gl = dependency('gl')
jsoncpp = dependency('jsoncpp', required : true)
curl = dependency('libcurl', required : true)


files = files(
    'main.cpp',
    '/home/hachikuji/Documents/imgui/implot/implot.cpp',
    '/home/hachikuji/Documents/imgui/implot/implot_items.cpp',
    '/home/hachikuji/Documents/imgui/imgui.cpp',
    '/home/hachikuji/Documents/imgui/imgui_draw.cpp',
    '/home/hachikuji/Documents/imgui/imgui_tables.cpp',
    '/home/hachikuji/Documents/imgui/imgui_demo.cpp',
    '/home/hachikuji/Documents/imgui/imgui_widgets.cpp',
    '/home/hachikuji/Documents/imgui/backends/imgui_impl_glfw.cpp',
    '/home/hachikuji/Documents/imgui/backends/imgui_impl_opengl3.cpp'
)

executable('air_monitor', files, include_directories : incdir, dependencies : [glfw_lib, gl, jsoncpp, curl])