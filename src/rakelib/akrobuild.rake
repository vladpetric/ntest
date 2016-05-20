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

$HEADER_EXTENSIONS = [".h", ".hpp"]
$CPP_EXTENSIONS = [".c", ".cc", ".cpp", ".cxx", ".c++"]

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

  # Maps header file to its corresponding cpp file, if it exists
  # E.g., a/b/c.h maps to a/b/c.cpp, if a/b/c.cpp exists, otherwise nil
  def FileMapper.map_header_to_cpp(path)
    rel_path = Util.make_relative_path(path)
    # file is not local
    return nil if rel_path.nil?
    srcs = $HEADER_EXTENSIONS.keep_if{|ext| path.end_with?(ext)}.collect{ |ext|
      base_path = path[0..-ext.length-1]
      $CPP_EXTENSIONS.map{|cppext| base_path + cppext}.keep_if{|file| File.exists?(file)}
    }.flatten.uniq
    raise "Multiple sources for base name #{path}: #{srcs.join(' ')}" if srcs.length > 1
    srcs.length == 0? nil : srcs[0]
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
      #Using backticks as Rake's sh outputs the command. Don't want that here.
      deps = `#{$COMPILER} #{$COMPILE_FLAGS} #{$MODE_COMPILE_FLAGS[mode]} -M #{src}`
      raise "Dependency determination failed for #{src}" if $?.to_i != 0
      # NOTE(vlad): spaces within included filenames are not supported
      # Get rid of \ at the end of lines, and also of the newline
      deps.gsub!(/\\\n/, '')
      # also get rid of <filename>: at the beginning
      deps[/^[^:]*:(.*)$/, 1].split(' ').each do |line|
        # Output either a relative path if the file is local, or the original line.
        output << (Util.make_relative_path(line.strip) || line) << "\n"
      end
      output.close
      success = true
    ensure
      FileUtils.rm(dc) if !success
    end
  end

  def Builder.compile_object(src, obj)
    mode = FileMapper.get_mode(obj)
    basedir, _ = File.split(obj)
    FileUtils.mkdir_p(basedir)
    RakeFileUtils::sh("#{$COMPILER} #{$COMPILE_FLAGS} #{$MODE_COMPILE_FLAGS[mode]} -c #{src} -o #{obj}") do |ok, res|
      raise "Compilation failed for #{src}" if !ok
    end
  end
  def recursive_invoke_and_collect(mode, top_level_srcs)

  end
  def Builder.collect_dependencies(mode, top_level_srcs)
    top_levels_srcs.each do |src|
      Rake::Task[]
    end 
  end
end

rule ".depcache" => ->(dc){
  [FileMapper.map_dc_to_cpp(dc)] + (File.exists?(dc) ? File.readlines(dc).map{|line| line.strip}: [])
} do |task|
  src = FileMapper.map_dc_to_cpp(task.name)
  Builder.create_depcache(src, task.name)
end

rule ".o" => ->(obj){
  src = FileMapper.map_obj_to_cpp(obj)
  raise "No source for object file #{obj}" if src.nil?
  dc = FileMapper.map_obj_to_dc(obj)
  [src, dc] + (File.exists?(dc) ? File.readlines(dc).map{|line| line.strip}: [])
} do |task|
  src = FileMapper.map_obj_to_cpp(task.name)
  Builder.compile_object(src, task.name)
end

task :clean do
  FileUtils::rm_rf(".akro/")
  $MODES.each{|mode| FileUtils::rm_rf("#{mode}/")}
end

task :default do
  puts FileMapper.map_header_to_cpp("options.h")
  puts FileMapper.map_header_to_cpp("/usr/include/assert.h").nil?
end
