#include "progresswidget.h"
#include "ui_progresswidget.h"
#include "appdata.h"
#include <QDebug>
#include <QTimer>
#include <QScrollBar>

ProgressWidget::ProgressWidget(QWidget *parent):
QWidget(parent),
ui(new Ui::ProgressWidget)
{
  ui->setupUi(this);
  ui->info->hide();
  ui->checkBox->hide();
  ui->textEdit->hide();

#ifdef __APPLE__
  QFont newFont("Courier", 13);
  ui->textEdit->setFont(newFont);
  ui->textEdit->setAttribute(Qt::WA_MacNormalSize);
#elif defined WIN32 || !defined __GNUC__
  QFont newFont("Courier", 9);
  ui->textEdit->setFont(newFont);
#endif
}

ProgressWidget::~ProgressWidget()
{
  delete ui;
}

void ProgressWidget::forceOpen()
{
  ui->checkBox->hide();
  ui->textEdit->show();
}

void ProgressWidget::setInfo(const QString &text)
{
  ui->info->show();
  ui->info->setText(text);
}

void ProgressWidget::setMaximum(int value)
{
  ui->progressBar->setMaximum(value);
}

int ProgressWidget::maximum()
{
  return ui->progressBar->maximum();
}

void ProgressWidget::setValue(int value)
{
  ui->progressBar->setValue(value);
}

void ProgressWidget::addText(const QString &text)
{
  ui->checkBox->setVisible(true);

  if (g.outputDisplayDetails()) {
    ui->checkBox->setChecked(true);
    ui->textEdit->setVisible(true);
  }

  QTextCursor cursor(ui->textEdit->textCursor());

  // is the scrollbar at the end?
  bool atEnd = (ui->textEdit->verticalScrollBar()->value() == ui->textEdit->verticalScrollBar()->maximum());

  cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 1);
  cursor.insertText(text);

  if (atEnd) {
    ui->textEdit->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
  }
}

QString ProgressWidget::getText()
{
  return ui->textEdit->toPlainText();
}

void ProgressWidget::setProgressColor(const QColor &color)
{
  ui->progressBar->setStyleSheet(QString("QProgressBar  {text-align: center;} QProgressBar::chunk { background-color: %1; text-align:center;}:").arg(color.name()));
}

#define HLINE_SEPARATOR "================================================================================="

void ProgressWidget::addSeparator()
{
  addText("\n" HLINE_SEPARATOR "\n");
}

void ProgressWidget::on_checkBox_toggled(bool checked)
{
  g.outputDisplayDetails(checked);
  ui->textEdit->setVisible(checked);
  QTimer::singleShot(0, this, SLOT(shrink()));
}

void ProgressWidget::lock(bool lock)
{
  emit locked(lock);
}

void ProgressWidget::shrink()
{
  emit detailsToggled();
}


