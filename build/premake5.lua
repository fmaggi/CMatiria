output_dir = '%{cfg.buildcfg}_%{cfg.architecture}_%{cfg.system}'

workspace 'Matiria'
	startproject		'Tests'
	architecture		'x64'
	configurations		{ 'Debug', 'Development', 'Release' }
	flags				'MultiProcessorCompile'
	language			'C'
	cdialect			'C11'
	conformancemode		'On'
	exceptionhandling	'Off'
	warnings			'Extra'
	floatingpoint		'Fast'
	staticruntime		'On'
	files				{
							'%{prj.name}/**.c',
							'%{prj.name}/**.h',
						}
	debugdir			('bin/'	 .. output_dir .. '/%{prj.name}')
	targetdir			('bin/'	 .. output_dir .. '/%{prj.name}')
	objdir				('bin_int/' .. output_dir .. '/%{prj.name}')

	filter 'Debug'
		symbols			'On'
		runtime			'Debug'

	filter 'Development'
		symbols			'On'
		optimize		'Speed'
		runtime			'Debug'
		flags			'LinkTimeOptimization'

	filter 'Release'
		optimize		'Speed'
		runtime			'Release'
		flags			'LinkTimeOptimization'
		defines			'NDEBUG'

	filter "system:windows"
		systemversion "latest"

	filter "system:linux"
		toolset("clang")

project 'Matiria'
	location			'%{prj.name}'
	kind				'StaticLib'
	includedirs			{ '', '%{prj.name}'}

project 'Tests'
	location			'%{prj.name}'
	kind				'ConsoleApp'
	includedirs			{ '', '%{prj.name}', 'Matiria' }
	links				'Matiria'
