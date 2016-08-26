require 'set'
all_tests = Set.new
all_tests_list = []
$MODES.each do |mode|
  task "#{mode}_test"
end

$AKRO_TESTS.each do |test|
  raise "Test name may not be nil" if test.name.nil?
  raise "Multiple tests with the same name: #{test.name}" if all_tests.include?(test.name)
  all_tests << test.name
  all_tests_list << test
  task "#{mode}/#{test.name}" => ->(name) do
    
  end
  $MODES.each do |mode|
    raise "Test name may not start with a build mode #{test.name}" if test.name.start_with?(mode)
    Rake::Task["#{mode}_test"].enhance("#{mode}/#{test.name}")
  end
end

