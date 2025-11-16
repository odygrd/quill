import re
import json
import urllib.parse


def generate_chart_url(markdown_table, title):
    # Define a static color mapping with bright and distinct colors
    color_mapping = {
        'Quill Bounded Dropping Queue': 'rgba(0,128,0,0.6)',  # Green
        'fmtlog': 'rgba(255,99,132,0.6)',  # Light Red
        'Quill Unbounded Queue': 'rgba(0,255,0,0.6)',  # Bright Green
        'PlatformLab NanoLog': 'rgba(255,159,64,0.6)',  # Orange
        'MS BinLog': 'rgba(153,102,255,0.6)',  # Purple
        'XTR': 'rgba(255,206,86,0.6)',  # Yellow
        'Reckless': 'rgba(255,99,71,0.6)',  # Tomato Red
        'Iyengar NanoLog': 'rgba(100,149,237,0.6)',  # Cornflower Blue
        'spdlog': 'rgba(0,128,128,0.6)',  # Teal
        'g3log': 'rgba(255,165,0,0.6)',  # Orange Red
        'BqLog': 'rgba(75,0,130,0.6)',  # Indigo
        'Quill Unbounded Queue - Macro Free Mode': 'rgba(0,206,209,0.6)',  # Dark Turquoise
        'Boost.Log': 'rgba(106,90,205,0.6)'  # Slate Blue
    }

    # Parse the markdown table
    lines = markdown_table.strip().split("\n")[2:]  # Skip header lines
    labels = ['50th', '75th', '90th', '95th', '99th', '99.9th']
    datasets = []

    # Process each row in the table
    for line in lines:
        parts = re.split(r'\s*\|\s*', line.strip('| '))
        library_name = re.sub(r'\[|\]\(.*?\)', '', parts[0])  # Remove markdown link syntax
        data = [int(x) for x in parts[1:]]

        # Cap data values at 100 ns
        capped_data = [min(x, 100) for x in data]

        dataset = {
            'label': library_name,
            'data': capped_data,
            'backgroundColor': color_mapping.get(library_name, 'rgba(0,0,0,0.6)'),  # Default color
            'barPercentage': 0.9,  # Adjust bar width
            'categoryPercentage': 0.9  # Adjust spacing between bars
        }
        datasets.append(dataset)

    # Construct the QuickChart URL
    chart_config = {
        'type': 'bar',
        'data': {
            'labels': labels,
            'datasets': datasets
        },
        'options': {
            'title': {
                'display': True,
                'text': title
            },
            'scales': {
                'yAxes': [{
                    'scaleLabel': {
                        'display': True,
                        'labelString': 'Latency (ns)'
                    },
                    'ticks': {
                        'max': 100
                    }
                }],
                'xAxes': [{
                    'scaleLabel': {
                        'display': True,
                        'labelString': 'Percentiles'
                    }
                }]
            },
            'plugins': {
                'datalabels': {
                    'display': False
                }
            }
        }
    }

    base_url = "https://quickchart.io/chart"
    chart_json = json.dumps(chart_config)
    encoded_chart = urllib.parse.quote(chart_json)
    chart_url = f"{base_url}?c={encoded_chart}"

    return chart_url


# Define multiple markdown tables and titles
markdown_tables = [
    """
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  8   |  8   |  9   |  9   |  11  |   13   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  8   |  8   |  9   |  9   |  10  |   13   |
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  8   |  9   |  10  |  10  |  12  |   13   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  13  |  14  |  16  |  18  |  21  |   26   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  20  |  21  |  21  |  22  |  59  |   94   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  27  |  27  |  28  |  29  |  31  |   33   |
| [XTR](https://github.com/choll/xtr)                                       |  7   |  7   |  29  |  31  |  33  |   55   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  25  |  27  |  30  |  31  |  33  |   37   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  87  |  99  | 122  | 129  | 165  |  228   |
| [spdlog](http://github.com/gabime/spdlog)                                 | 145  | 148  | 152  | 154  | 162  |  173   |
| [g3log](http://github.com/KjellKod/g3log)                                 | 773  | 779  | 784  | 788  | 801  |  963   |
| [Boost.Log](https://www.boost.org)                                        | 829  | 835  | 841  | 848  | 892  |  955   |
    """,
    """
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  8   |  9   |  9   |  10  |  13  |   15   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  8   |  8   |  9   |  10  |  12  |   14   |
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  9   |  9   |  9   |  10  |  12  |   13   |
| [XTR](https://github.com/choll/xtr)                                       |  7   |  8   |  9   |  10  |  31  |   38   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  9   |  13  |  16  |  17  |  22  |   26   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  21  |  21  |  22  |  23  |  60  |  101   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  18  |  22  |  25  |  26  |  29  |   43   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  28  |  29  |  30  |  31  |  34  |   45   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  57  |  90  | 122  | 131  | 175  |  1507  |
| [spdlog](http://github.com/gabime/spdlog)                                 | 213  | 249  | 314  | 346  | 430  |  710   |
| [Boost.Log](https://www.boost.org)                                        | 1615 | 2522 | 2780 | 2884 | 4142 |  5070  |
| [g3log](http://github.com/KjellKod/g3log)                                 | 1339 | 4106 | 4508 | 4651 | 6089 |  7394  |
    """,
    """
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  10  |  12  |  13  |  13  |  15  |   16   |
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  12  |  13  |  14  |  15  |  16  |   19   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  12  |  13  |  14  |  15  |  17  |   19   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  23  |  23  |  24  |  25  |  62  |   99   |
| [XTR](https://github.com/choll/xtr)                                       |  7   |  8   |  28  |  29  |  33  |   53   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  15  |  18  |  29  |  32  |  37  |   43   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  31  |  32  |  33  |  34  |  36  |   37   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  90  | 105  | 112  | 115  | 121  |  129   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  89  |  99  | 122  | 131  | 181  |  347   |
| [spdlog](http://github.com/gabime/spdlog)                                 | 129  | 133  | 138  | 142  | 149  |  159   |
| [g3log](http://github.com/KjellKod/g3log)                                 | 562  | 566  | 570  | 572  | 582  |  749   |
| [Boost.Log](https://www.boost.org)                                        | 634  | 638  | 641  | 643  | 647  |  662   |
    """,
    """
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  10  |  12  |  13  |  13  |  15  |   19   |
| [XTR](https://github.com/choll/xtr)                                       |  9   |  11  |  13  |  15  |  30  |   41   |
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  13  |  14  |  16  |  16  |  20  |   23   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  12  |  13  |  15  |  16  |  19  |   21   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  11  |  16  |  20  |  26  |  39  |   47   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  23  |  24  |  25  |  26  |  64  |  103   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  31  |  33  |  35  |  36  |  39  |   45   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  79  |  92  | 100  | 104  | 111  |  125   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  56  |  91  | 124  | 131  | 179  |  1458  |
| [spdlog](http://github.com/gabime/spdlog)                                 | 196  | 224  | 281  | 313  | 395  |  670   |
| [Boost.Log](https://www.boost.org)                                        | 1363 | 2409 | 2604 | 2649 | 3919 |  5606  |
| [g3log](http://github.com/KjellKod/g3log)                                 | 1041 | 3876 | 4271 | 4444 | 5708 |  6774  |
    """,
    """
| Library                                                        | 50th  | 75th  | 90th  | 95th  | 99th  | 99.9th |
|----------------------------------------------------------------|:-----:|:-----:|:-----:|:-----:|:-----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  46   |  49   |  53   |  55   |  59   |   64   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  66   |  68   |  70   |  72   |  77   |  276   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  141  |  152  |  161  |  166  |  173  |  182   |
| [XTR](https://github.com/choll/xtr)                            |  284  |  294  |  340  |  346  |  354  |  586   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  641  |  655  |  689  |  711  |  733  |  765   |
| [spdlog](http://github.com/gabime/spdlog)                      | 6299  | 6361  | 6420  | 6463  | 6683  |  7348  |
| [Boost.Log](https://www.boost.org)                             | 33432 | 33643 | 33834 | 33966 | 34392 | 34951  |
    """,
    """
| Library                                                        | 50th  | 75th  |  90th  |  95th  |  99th  | 99.9th |
|----------------------------------------------------------------|:-----:|:-----:|:------:|:------:|:------:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  49   |  53   |   57   |   60   |   65   |   77   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  70   |  72   |   75   |   77   |   86   |  291   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  84   |  90   |   97   |  102   |  112   |  125   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  682  |  724  |  755   |  770   |  803   |  936   |
| [XTR](https://github.com/choll/xtr)                            |  575  | 1218  |  1308  |  1362  | 191866 | 217427 |
| [spdlog](http://github.com/gabime/spdlog)                      | 6583  | 6660  |  6738  |  6804  |  7612  |  9209  |
| [Boost.Log](https://www.boost.org)                             | 35919 | 77700 | 150238 | 175931 | 271214 | 354942 |
    """
]

titles = ["Logging Numbers - 1 Thread Logging", "Logging Numbers - 4 Thread Logging",
          "Logging Large Strings - 1 Thread Logging", "Logging Large Strings - 4 Thread Logging",
          "Logging Vector - 1 Thread Logging", "Logging Vector - 4 Thread Logging"]

# Generate URLs for each chart
chart_urls = []
for markdown_table, title in zip(markdown_tables, titles):
    url = generate_chart_url(markdown_table, title)
    chart_urls.append(url)

# Print all generated URLs
for i, url in enumerate(chart_urls, start=1):
    print(f"{url}\n\n")
