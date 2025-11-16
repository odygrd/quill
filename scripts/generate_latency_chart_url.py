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
        # capped_data = [min(x, 100) for x in data]

        dataset = {
            'label': library_name,
            'data': data,
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
                    'type': 'logarithmic',
                    'scaleLabel': {
                        'display': True,
                        'labelString': 'Latency (ns)'
                    },
                    'ticks': {
                        'min': 1,
                        'maxTicksLimit': 10,
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
| Library                                                                  | 50th | 75th | 90th | 95th | 99th | 99.9th |
|--------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)          |  8   |  8   |  9   |  9   |  10  |   12   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                 |  8   |  8   |  9   |  9   |  11  |   13   |
| [fmtlog](https://github.com/MengRao/fmtlog)                              |  8   |  9   |  9   |  10  |  12  |   13   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)            |  13  |  14  |  16  |  18  |  21  |   24   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                    |  21  |  21  |  21  |  22  |  59  |   93   |
| [Quill Unbounded Queue (Log Functions)](https://github.com/odygrd/quill) |  27  |  28  |  29  |  30  |  31  |   33   |
| [XTR](https://github.com/choll/xtr)                                      |  7   |  7   |  29  |  31  |  33  |   55   |
| [Reckless](https://github.com/mattiasflodin/reckless)                    |  26  |  28  |  31  |  32  |  34  |   40   |
| [BqLog](https://github.com/Tencent/BqLog)                                |  42  |  42  |  43  |  75  |  83  |   94   |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                 |  87  |  98  | 122  | 129  | 209  |  381   |
| [spdlog](https://github.com/gabime/spdlog)                               | 145  | 148  | 151  | 154  | 162  |  171   |
| [g3log](https://github.com/KjellKod/g3log)                               | 775  | 781  | 787  | 791  | 805  |  967   |
| [Boost.Log](https://www.boost.org)                                       | 838  | 845  | 853  | 859  | 897  |  959   |
    """,
    """
| Library                                                                  | 50th | 75th | 90th | 95th | 99th | 99.9th |
|--------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                 |  8   |  9   |  9   |  9   |  12  |   15   |
| [fmtlog](https://github.com/MengRao/fmtlog)                              |  8   |  9   |  9   |  10  |  12  |   13   |
| [XTR](https://github.com/choll/xtr)                                      |  7   |  8   |  9   |  10  |  32  |   39   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)          |  8   |  9   |  10  |  11  |  13  |   15   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)            |  9   |  13  |  15  |  17  |  22  |   26   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                    |  21  |  22  |  22  |  23  |  61  |   98   |
| [Reckless](https://github.com/mattiasflodin/reckless)                    |  19  |  23  |  26  |  27  |  31  |   57   |
| [Quill Unbounded Queue (Log Functions)](https://github.com/odygrd/quill) |  28  |  29  |  30  |  31  |  33  |   44   |
| [BqLog](https://github.com/Tencent/BqLog)                                |  47  |  49  |  50  | 122  | 143  |  4248  |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                 |  58  |  90  | 124  | 132  | 178  |  1478  |
| [spdlog](https://github.com/gabime/spdlog)                               | 214  | 260  | 327  | 359  | 452  |  754   |
| [Boost.Log](https://www.boost.org)                                       | 1640 | 2538 | 2793 | 2896 | 4150 |  4962  |
| [g3log](https://github.com/KjellKod/g3log)                               | 1336 | 4102 | 4501 | 4641 | 6079 |  7363  |
    """,
    """
| Library                                                                  | 50th | 75th | 90th | 95th | 99th | 99.9th |
|--------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [fmtlog](https://github.com/MengRao/fmtlog)                              |  10  |  12  |  13  |  13  |  15  |   17   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                 |  11  |  12  |  14  |  14  |  16  |   18   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)          |  12  |  13  |  14  |  15  |  16  |   19   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                    |  23  |  23  |  24  |  25  |  62  |   96   |
| [XTR](https://github.com/choll/xtr)                                      |  8   |  8   |  28  |  29  |  33  |   52   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)            |  15  |  19  |  29  |  32  |  37  |   43   |
| [Quill Unbounded Queue (Log Functions)](https://github.com/odygrd/quill) |  31  |  33  |  34  |  35  |  36  |   38   |
| [BqLog](https://github.com/Tencent/BqLog)                                |  44  |  45  |  46  |  76  |  85  |   93   |
| [Reckless](https://github.com/mattiasflodin/reckless)                    |  89  | 105  | 111  | 114  | 120  |  131   |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                 |  89  | 100  | 123  | 132  | 205  |  367   |
| [spdlog](https://github.com/gabime/spdlog)                               | 129  | 133  | 137  | 141  | 149  |  157   |
| [g3log](https://github.com/KjellKod/g3log)                               | 565  | 568  | 571  | 573  | 582  |  740   |
| [Boost.Log](https://www.boost.org)                                       | 638  | 641  | 645  | 647  | 651  |  659   |
    """,
    """
| Library                                                                  | 50th | 75th | 90th | 95th | 99th | 99.9th |
|--------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [fmtlog](https://github.com/MengRao/fmtlog)                              |  10  |  12  |  13  |  13  |  16  |   19   |
| [XTR](https://github.com/choll/xtr)                                      |  8   |  11  |  13  |  15  |  30  |   40   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)                 |  13  |  14  |  15  |  16  |  19  |   21   |
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill)          |  13  |  15  |  16  |  17  |  20  |   22   |
| [PlatformLab NanoLog](https://github.com/PlatformLab/NanoLog)            |  11  |  16  |  20  |  24  |  38  |   46   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)                    |  23  |  24  |  25  |  26  |  65  |  104   |
| [Quill Unbounded Queue (Log Functions)](https://github.com/odygrd/quill) |  31  |  33  |  35  |  36  |  39  |   46   |
| [Reckless](https://github.com/mattiasflodin/reckless)                    |  77  |  92  | 101  | 105  | 112  |  126   |
| [BqLog](https://github.com/Tencent/BqLog)                                |  51  |  55  |  59  | 119  | 136  |  2973  |
| [Iyengar NanoLog](https://github.com/Iyengar111/NanoLog)                 |  55  |  86  | 103  | 128  | 170  |  1364  |
| [spdlog](https://github.com/gabime/spdlog)                               | 196  | 227  | 281  | 310  | 395  |  690   |
| [Boost.Log](https://www.boost.org)                                       | 1357 | 2420 | 2607 | 2651 | 3939 |  5682  |
| [g3log](https://github.com/KjellKod/g3log)                               | 1050 | 3886 | 4271 | 4442 | 5693 |  6831  |
    """,
    """
| Library                                                         | 50th  | 75th  | 90th  | 95th  | 99th  | 99.9th |
|-----------------------------------------------------------------|:-----:|:-----:|:-----:|:-----:|:-----:|:------:|
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill) |  46   |  49   |  52   |  55   |  59   |   65   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)           |  67   |  69   |  71   |  73   |  79   |  279   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)        |  139  |  150  |  160  |  165  |  173  |  183   |
| [XTR](https://github.com/choll/xtr)                             |  286  |  297  |  341  |  346  |  353  |  589   |
| [fmtlog](https://github.com/MengRao/fmtlog)                     |  646  |  660  |  685  |  705  |  733  |  767   |
| [spdlog](https://github.com/gabime/spdlog)                      | 6301  | 6363  | 6419  | 6455  | 6802  |  7414  |
| [Boost.Log](https://www.boost.org)                              | 34432 | 34623 | 34797 | 34928 | 35703 | 36233  |
    """,
    """
| Library                                                         | 50th  | 75th  |  90th  |  95th  |  99th  | 99.9th |
|-----------------------------------------------------------------|:-----:|:-----:|:------:|:------:|:------:|:------:|
| [Quill Bounded Dropping Queue](https://github.com/odygrd/quill) |  49   |  53   |   57   |   60   |   65   |   79   |
| [MS BinLog](https://github.com/Morgan-Stanley/binlog)           |  69   |  72   |   74   |   76   |   82   |  292   |
| [Quill Unbounded Queue](https://github.com/odygrd/quill)        |  84   |  91   |   98   |  103   |  113   |  130   |
| [fmtlog](https://github.com/MengRao/fmtlog)                     |  675  |  701  |  740   |  756   |  779   |  809   |
| [XTR](https://github.com/choll/xtr)                             |  627  | 1225  |  1317  |  1376  | 205300 | 287631 |
| [spdlog](https://github.com/gabime/spdlog)                      | 6593  | 6671  |  6751  |  6815  |  7610  |  8794  |
| [Boost.Log](https://www.boost.org)                              | 36322 | 80294 | 151775 | 177681 | 272649 | 362401 |
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
