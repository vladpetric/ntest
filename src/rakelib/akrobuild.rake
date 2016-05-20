require 'tempfile'

$COMPILER = "g++"
$COMPILE_FLAGS = "-std=c++14 -pthread -Wall -Werror"
$MODE_COMPILE_FLAGS = {
  "debug" => "-g3",
  "release" => "-O3 -g3"
}
$MODES = $MODE_COMPILE_FLAGS.keys

$LINKER = $COMPILER
$LINKER_FLAGS = $COMPILE_FLAGS
$MODE_LINKER_FLAGS = $MODE_COMPILE_FLAGS

$CPP_EXTENSIONS = [".cc", ".cpp", ".cxx", ".c++"]

module Util
  def Util.make_relative_path(path)
    absolute_path = File.absolute_path(path)
    base_dir = File.absolute_path(Dir.pwd)
    base_dir << '/' if !base_dir.end_with?('/')
    r = absolute_path.start_with?(base_dir) ? absolute_path[base_dir.size..-1] : nil
    if !r.nil? && r.start_with?('/')
      raise "Relative path #{r} is not relative"
    end
    r
  end
  def Util.read_file_list(path)
    File.readlines(path)
  end

end

module FileMapper
  # Extract build mode from path.
  # E.g., debug/a/b/c.o returns debug.
  def FileMapper.get_mode(path)
    rel_path = Util.make_relative_path(path)
    raise "Path #{path} does not belong to #{Dir.pwd}" if rel_path.nil?
    mode = rel_path[/^([^\/]*)/, 1]
    raise "Unknown mode #{mode} for #{path}" if !$MODES.include?(mode)
    mode
  end
  def FileMapper.get_mode_from_dc(path)
    rel_path = Util.make_relative_path(path)
    raise "Path #{path} does not belong to #{Dir.pwd}" if rel_path.nil?
    mode = rel_path[/^\.akro\/([^\/]*)/, 1]
    raise "Unknown mode #{mode} for #{path}" if !$MODES.include?(mode)
    mode
  end
  # Strip the build mode from path.
  def FileMapper.strip_mode(path)
    rel_path = Util.make_relative_path(path)
    raise "Path #{path} does not belong to #{Dir.pwd}" if rel_path.nil?
    get_mode(rel_path) # for sanity checking
    rel_path[/^[^\/]*\/(.*)$/, 1]
  end
  # Maps object file to its corresponding depcache file.
  # E.g., release/a/b/c.o maps to .akro/release/a/b/c.depcache
  def FileMapper.map_obj_to_dc(path)
    FileMapper.get_mode(path)
    raise "#{path} is not a .o file" if !path.end_with?('.o')
    ".akro/#{path[0..-'.o'.length-1]}.depcache"
  end
  # Maps object file to its corresponding cpp file, if it exists.
  # E.g., release/a/b/c.o maps to a/b/c{.cpp,.cc,.cxx,.c++}
  def FileMapper.map_obj_to_cpp(path)
    raise "#{path} is not a .o file" if !path.end_with?('.o')
    file = FileMapper.strip_mode(path)
    file = file[0..-'.o'.length-1]   
    srcs = $CPP_EXTENSIONS.map{|ext| file + ext}.keep_if{|file| File.exists?(file)}
    raise "Multiple sources for base name #{file}: #{srcs.join(' ')}" if srcs.length > 1
    srcs.length == 0? nil : srcs[0]
  end
  # Maps depcache file to its corresponding cpp file, which should exist.
  # E.g., .akro/release/a/b/c.o maps to a/b/c{.cpp,.cc,.cxx,.c++}
  def FileMapper.map_dc_to_cpp(path)
    raise "#{path} is not a .depcache file" if !path.end_with?('.depcache') || !path.start_with?('.akro')
    file = path[/^\.akro\/(.*)\.depcache$/, 1]
    file = FileMapper.strip_mode(file)
    srcs = $CPP_EXTENSIONS.map{|ext| file + ext}.keep_if{|file| File.exists?(file)}
    raise "Multiple sources for base name #{file}: #{srcs.join(' ')}" if srcs.length > 1
    raise "No sources for base name #{file}" if srcs.length == 0
    srcs[0]
  end
end

#Builder encapsulates the compilation/linking/dependecy checking functionality
module Builder
  def Builder.create_depcache(src, dc)
    success = false
    mode = FileMapper.get_mode_from_dc(dc)
    basedir, _ = File.split(dc)
    FileUtils.mkdir_p(basedir)
    output = File.open(dc, "w")
    begin
      tmpfile = Tempfile.open('depcache') do |temp|
        temp.close
        RakeFileUtils::sh("#{$COMPILER} #{$COMPILE_FLAGS} #{$MODE_COMPILE_FLAGS[mode]} -M #{src} -o #{temp.path}") do |ok, res|
          if ok
            deps = File.read(temp.path)
            deps.gsub!('\\\n', '') # get rid of \ at the end of lines, and also of the newline
            puts "Deps: <#{deps}>"
            deps[/^[^:]*:(.*)$/, 1].split(' ').each do |line|
              puts "Dep: <#{line}>"
              line.strip!
              relpath = Util.make_relative_path(line)
              output << (relpath || line)
            end
            output.close
            success = true
          end
        end
      end
    ensure
      if !success
        puts "Failed"
        output.close
        FileUtils.rm(dc)
        raise "Dependency determination failed for #{src}"
      end
    end
  end
end

rule ".depcache" => ->(dc){
  [FileMapper.map_dc_to_cpp(dc)] + (File.exists?(dc) ? File.readlines(dc).map{|line| line.strip}: [])
} do |task|
  src = FileMapper.map_dc_to_cpp(task.name)
  Builder.create_depcache(src, task.name)
  puts "Attempting to create depcache: #{task.name}"
  raise "Stop"
end

rule ".o" => ->(obj){
  src = FileMapper.map_obj_to_cpp(obj)
  raise "No source for object file #{obj}" if src.nil?
  dc = FileMapper.map_obj_to_dc(obj)
  [src, dc] + (File.exists?(dc) ? File.readlines(dc).map{|line| line.strip}: [])
} do |task|
  mode = FileMapper.get_mode(task.name)
  src = FileMapper.map_obj_to_cpp(task.name)
  puts "#{mode} <#{src}>"
end

task :default do
end
