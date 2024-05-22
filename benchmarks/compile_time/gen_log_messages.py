import random


def generate_log_statements(num_statements):
    argument_types = [
        '1', '2', '3.0', '4.0f', '5L', '6LL', '7UL', '8ULL', 'true', 'false',
        '"example1"', '"example2"', '"example3"', 'std::string("str1")',
        'std::string("str2")', 'std::string_view("view1")', 'std::string_view("view2")',
        'static_cast<short>(9)', 'static_cast<unsigned short>(10)'
    ]
    random_words = ["quick", "brown", "fox", "jumps", "over", "lazy", "dog", "logging", "test", "example"]

    statements = []
    for i in range(num_statements):
        num_args = random.randint(1, 10)  # Number of arguments for the log statement
        args = random.sample(argument_types, num_args)
        placeholders = ' '.join(["{}" for _ in args])
        num_words = random.randint(3, 4)  # Number of random words in the log message
        words = ' '.join(random.sample(random_words, num_words))
        statement = f'  LOG_INFO(logger, "{words} {placeholders}", {", ".join(args)});'
        statements.append(statement)

    return statements


def write_to_file(filename, statements):
    with open(filename, 'w') as f:
        f.write('#include "quill/Backend.h"\n')
        f.write('#include "quill/Frontend.h"\n')
        f.write('#include "quill/LogMacros.h"\n')
        f.write('#include "quill/Logger.h"\n')
        f.write('#include "quill/sinks/ConsoleSink.h"\n')
        f.write('#include <string>\n')
        f.write('#include <utility>\n\n')
        f.write('/**\n')
        f.write(' * Trivial logging example to console\n')
        f.write(' */\n\n')
        f.write('int main()\n')
        f.write('{\n')
        f.write('  // Start the backend thread\n')
        f.write('  quill::BackendOptions backend_options;\n')
        f.write('  quill::Backend::start(backend_options);\n\n')
        f.write('  // Frontend\n')
        f.write('  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");\n')
        f.write('  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));\n\n')

        for statement in statements:
            f.write(f'{statement}\n')

        f.write('\n  return 0;\n')
        f.write('}\n')


if __name__ == '__main__':
    num_statements = 2000
    statements = generate_log_statements(num_statements)
    write_to_file('log_benchmark.cpp', statements)
