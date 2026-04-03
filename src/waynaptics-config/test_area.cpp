#include "test_area.h"
#include "test_button.h"

#include <QGroupBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>

static const char *kLoremIpsum =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod\n"
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim\n"
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea\n"
    "commodo consequat.\n"
    "\n"
    "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum\n"
    "dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non\n"
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n"
    "\n"
    "Curabitur pretium tincidunt lacus. Nulla gravida orci a odio. Nullam\n"
    "varius, turpis et commodo pharetra, est eros bibendum elit, nec luctus\n"
    "magna felis sollicitudin mauris. Integer in mauris eu nibh euismod\n"
    "gravida.\n"
    "\n"
    "Duis ac tellus et risus vulputate vehicula. Donec lobortis risus a elit.\n"
    "Etiam tempor. Ut ullamcorper, ligula ut dictum pharetra, nisi nunc\n"
    "fringilla magna, in commodo elit erat nec turpis. Ut pharetra augue nec\n"
    "augue.\n"
    "\n"
    "Nam elit agna, endrerit sit amet, tincidunt ac, viverra sed, nulla.\n"
    "Donec porta diam eu massa. Quisque diam lorem, interdum vitae, dapibus\n"
    "ac, scelerisque vitae, pede.\n"
    "\n"
    "Maecenas malesuada elit lectus felis, malesuada ultricies. Curabitur et\n"
    "ligula. Ut molestie a, ultricies porta urna. Vestibulum commodo volutpat\n"
    "a, convallis ac, laoreet enim.\n"
    "\n"
    "Phasellus fermentum in, dolor. Pellentesque facilisis. Nulla imperdiet\n"
    "sit amet magna. Vestibulum dapibus, mauris nec malesuada fames ac turpis\n"
    "velit, rhoncus eu, luctus et interdum adipiscing wisi. Aliquam erat ac\n"
    "ipsum. Integer aliquam purus.\n"
    "\n"
    "Fusce suscipit, wisi nec facilisis facilisis, est dui fermentum leo,\n"
    "quis tempor ligula erat quis odio. Nunc porta vulputate tellus. Nunc\n"
    "rutrum turpis sed pede. Sed bibendum. Aliquam posuere.\n"
    "\n"
    "Nunc aliquet, augue nec adipiscing interdum, lacus tellus malesuada\n"
    "massa, quis varius mi purus non odio. Pellentesque condimentum, magna\n"
    "ut suscipit hendrerit, ipsum augue ornare nulla, non luctus diam neque\n"
    "sit amet urna.\n"
    "\n"
    "Curabitur vulputate vestibulum lorem. Fusce sagittis, libero non\n"
    "molestie mollis, magna orci ullamcorper dolor, at vulputate neque nulla\n"
    "lacinia eros. Sed id ligula quis est convallis tempor.\n"
    "\n"
    "Praesent dapibus, neque id cursus faucibus, tortor neque egestas augue,\n"
    "eu vulputate magna eros eu erat. Aliquam erat volutpat. Nam dui mi,\n"
    "tincidunt quis, accumsan porttitor, facilisis luctus, metus.\n"
    "\n"
    "Phasellus ultrices nulla quis nibh. Quisque a lectus. Donec consectetuer\n"
    "ligula vulputate sem tristique cursus. Nam nulla quam, gravida non,\n"
    "commodo a, sodales sit amet, nisi.\n"
    "\n"
    "Aenean lectus. Praesent vel mi. Sed aliquam nulla non nisl scelerisque\n"
    "elementum. Cras nisi. Cras posuere.\n"
    "\n"
    "Donec vitae dolor. Nullam tristique diam non turpis. Cras placerat\n"
    "accumsan nulla. Nullam rutrum. Nam vestibulum accumsan nisl.\n"
    "\n"
    "This is a very long line that should definitely extend beyond the visible area of the text widget and force horizontal scrolling to be necessary, demonstrating that horizontal scroll works correctly in the test area.\n"
    "\n"
    "Sed ut perspiciatis unde omnis iste natus error sit voluptatem\n"
    "accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab\n"
    "illo inventore veritatis et quasi architecto beatae vitae dicta sunt\n"
    "explicabo.\n"
    "\n"
    "Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut\n"
    "fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem\n"
    "sequi nesciunt.\n"
    "\n"
    "Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet,\n"
    "consectetur, adipisci velit, sed quia non numquam eius modi tempora\n"
    "incidunt ut labore et dolore magnam aliquam quaerat voluptatem.\n"
    "\n"
    "Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis\n"
    "suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis\n"
    "autem vel eum iure reprehenderit qui in ea voluptate velit esse quam\n"
    "nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo\n"
    "voluptas nulla pariatur?\n"
    "\n"
    "At vero eos et accusamus et iusto odio dignissimos ducimus qui\n"
    "blanditiis praesentium voluptatum deleniti atque corrupti quos dolores\n"
    "et quas molestias excepturi sint occaecati cupiditate non provident,\n"
    "similique sunt in culpa qui officia deserunt mollitia animi, id est\n"
    "laborum et dolorum fuga.\n"
    "\n"
    "Et harum quidem rerum facilis est et expedita distinctio. Nam libero\n"
    "tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo\n"
    "minus id quod maxime placeat facere possimus, omnis voluptas assumenda\n"
    "est, omnis dolor repellendus.\n"
    "\n"
    "Temporibus autem quibusdam et aut officiis debitis aut rerum\n"
    "necessitatibus saepe eveniet ut et voluptates repudiandae sint et\n"
    "molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente\n"
    "delectus, ut aut reiciendis voluptatibus maiores alias consequatur\n"
    "aut perferendis doloribus asperiores repellat.\n";

TestArea::TestArea(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *group = new QGroupBox(tr("Testing Area"), this);
    auto *groupLayout = new QVBoxLayout(group);

    // Button detector
    auto *buttonLabel = new QLabel(tr("Button detection:"), group);
    groupLayout->addWidget(buttonLabel);

    auto *testButton = new TestButton(group);
    testButton->setMinimumHeight(40);
    groupLayout->addWidget(testButton);

    // Drag/scroll test area (Lorem Ipsum text)
    auto *dragLabel = new QLabel(tr("Drag and scroll test:"), group);
    groupLayout->addWidget(dragLabel);

    auto *textEdit = new QTextEdit(group);
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);
    textEdit->setText(QString::fromUtf8(kLoremIpsum));
    textEdit->setMinimumHeight(200);
    groupLayout->addWidget(textEdit, 1);

    layout->addWidget(group);
}
