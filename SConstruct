
env = Environment()

includes = '''
              .
              ./cost-model/include
              ./cost-model/include/base
              ./cost-model/include/tools
              ./cost-model/include/user-api
              ./cost-model/include/dataflow-analysis
              ./cost-model/include/dataflow-specification-language
              ./cost-model/include/design-space-exploration
              ./cost-model/include/cost-analysis
              ./cost-model/include/abstract-hardware-model
              ./cost-model/src
              /opt/homebrew/Cellar/boost/1.82.0_1/include/boost
'''
env.Append(LINKFLAGS=['-lboost_program_options', '-lboost_filesystem', '-lboost_system'])
env.Append(CXXFLAGS=['-std=c++17', '-lboost_program_options',  '-lboost_filesystem', '-lboost_system'])
env.Append(LIBS=['-lboost_program_options',  '-lboost_filesystem', '-lboost_system' ])

env.Append(CPPPATH = Split(includes))
env.Append(CPPPATH=['/opt/homebrew/Cellar/boost/1.82.0_1/include/'])
env.Append(LIBPATH=['/opt/homebrew/lib'])
#env.Program("maestro-top.cpp")
#env.Program('maestro', ['maestro-top.cpp', 'lib/src/maestro_v3.cpp', 'lib/src/BASE_base-objects.cpp' ])
env.Program('maestro', ['maestro-top.cpp', 'cost-model/src/BASE_base-objects.cpp' ])
#env.Library('maestro', ['maestro-top.cpp', 'lib/src/maestro_v3.cpp', 'lib/src/BASE_base-objects.cpp' ])

