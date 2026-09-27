#!/usr/bin/env python3
"""Generate the Clock Plus technical reference PDF from Markdown."""

from __future__ import annotations

import html
import re
from pathlib import Path

from pypdf import PdfReader, PdfWriter
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    KeepTogether,
    PageBreak,
    PageTemplate,
    Paragraph,
    Preformatted,
    Spacer,
    Table,
    TableStyle,
)
from reportlab.platypus.tableofcontents import TableOfContents


ROOT = Path(__file__).resolve().parents[1]
SOURCE_MD = ROOT / "docs" / "clock-plus-technical-reference.md"
SCHEMATIC_PDF = ROOT / "docs" / "Clock i2c l0.pdf"
PCB_PDF = ROOT / "docs" / "Clock i2c l0 pcb.pdf"
TMP_DIR = ROOT / "tmp" / "pdfs" / "clock-plus-technical"
BODY_PDF = TMP_DIR / "clock-plus-technical-reference-body.pdf"
OUTPUT_PDF = ROOT / "output" / "pdf" / "clock-plus-technical-reference.pdf"

FONT_REGULAR = "/System/Library/Fonts/Supplemental/Arial.ttf"
FONT_BOLD = "/System/Library/Fonts/Supplemental/Arial Bold.ttf"
FONT_ITALIC = "/System/Library/Fonts/Supplemental/Arial Italic.ttf"
FONT_MONO = "/System/Library/Fonts/Supplemental/Courier New.ttf"
FONT_MONO_BOLD = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf"

INK = colors.HexColor("#17212B")
MUTED = colors.HexColor("#5E6B75")
ACCENT = colors.HexColor("#196A7A")
ACCENT_DARK = colors.HexColor("#124C58")
ACCENT_PALE = colors.HexColor("#EAF4F5")
LINE = colors.HexColor("#BCC9CE")
TABLE_ALT = colors.HexColor("#F3F7F8")
WARNING_BG = colors.HexColor("#FFF4D6")


def register_fonts() -> None:
    pdfmetrics.registerFont(TTFont("TechSans", FONT_REGULAR))
    pdfmetrics.registerFont(TTFont("TechSans-Bold", FONT_BOLD))
    pdfmetrics.registerFont(TTFont("TechSans-Italic", FONT_ITALIC))
    pdfmetrics.registerFont(TTFont("TechMono", FONT_MONO))
    pdfmetrics.registerFont(TTFont("TechMono-Bold", FONT_MONO_BOLD))
    pdfmetrics.registerFontFamily(
        "TechSans",
        normal="TechSans",
        bold="TechSans-Bold",
        italic="TechSans-Italic",
        boldItalic="TechSans-Bold",
    )


def inline_markup(value: str) -> str:
    escaped = html.escape(value.strip())
    escaped = re.sub(
        r"`([^`]+)`", r'<font name="TechMono" color="#124C58">\1</font>', escaped
    )
    escaped = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", escaped)
    escaped = re.sub(r"\*([^*]+)\*", r"<i>\1</i>", escaped)
    return escaped


def styles() -> dict[str, ParagraphStyle]:
    sample = getSampleStyleSheet()
    return {
        "body": ParagraphStyle(
            "Body",
            parent=sample["BodyText"],
            fontName="TechSans",
            fontSize=9.1,
            leading=12.2,
            textColor=INK,
            spaceAfter=4.5,
            allowWidows=0,
            allowOrphans=0,
        ),
        "h1": ParagraphStyle(
            "Heading1",
            parent=sample["Heading1"],
            fontName="TechSans-Bold",
            fontSize=17,
            leading=20,
            textColor=ACCENT_DARK,
            spaceBefore=12,
            spaceAfter=8,
            keepWithNext=1,
        ),
        "h2": ParagraphStyle(
            "Heading2",
            parent=sample["Heading2"],
            fontName="TechSans-Bold",
            fontSize=12.2,
            leading=15,
            textColor=ACCENT,
            spaceBefore=9,
            spaceAfter=5,
            keepWithNext=1,
        ),
        "h3": ParagraphStyle(
            "Heading3",
            parent=sample["Heading3"],
            fontName="TechSans-Bold",
            fontSize=10.2,
            leading=13,
            textColor=INK,
            spaceBefore=7,
            spaceAfter=4,
            keepWithNext=1,
        ),
        "bullet": ParagraphStyle(
            "Bullet",
            parent=sample["BodyText"],
            fontName="TechSans",
            fontSize=9,
            leading=12,
            leftIndent=11,
            firstLineIndent=-5,
            bulletIndent=2,
            spaceAfter=2.5,
            textColor=INK,
        ),
        "number": ParagraphStyle(
            "Number",
            parent=sample["BodyText"],
            fontName="TechSans",
            fontSize=9,
            leading=12,
            leftIndent=14,
            firstLineIndent=-8,
            spaceAfter=2.5,
            textColor=INK,
        ),
        "quote": ParagraphStyle(
            "Quote",
            parent=sample["BodyText"],
            fontName="TechSans",
            fontSize=9,
            leading=12,
            leftIndent=8,
            rightIndent=8,
            borderColor=colors.HexColor("#D49B28"),
            borderWidth=1.2,
            borderPadding=7,
            backColor=WARNING_BG,
            textColor=INK,
            spaceBefore=5,
            spaceAfter=7,
        ),
        "code": ParagraphStyle(
            "Code",
            parent=sample["Code"],
            fontName="TechMono",
            fontSize=7.4,
            leading=9.5,
            leftIndent=5,
            rightIndent=5,
            borderColor=LINE,
            borderWidth=0.5,
            borderPadding=6,
            backColor=colors.HexColor("#F5F7F8"),
            textColor=colors.HexColor("#15202A"),
            spaceBefore=4,
            spaceAfter=7,
        ),
        "table": ParagraphStyle(
            "TableCell",
            parent=sample["BodyText"],
            fontName="TechSans",
            fontSize=7.3,
            leading=9.1,
            textColor=INK,
        ),
        "table_header": ParagraphStyle(
            "TableHeader",
            parent=sample["BodyText"],
            fontName="TechSans-Bold",
            fontSize=7.4,
            leading=9.2,
            textColor=colors.white,
        ),
        "toc_title": ParagraphStyle(
            "TocTitle",
            parent=sample["Heading1"],
            fontName="TechSans-Bold",
            fontSize=20,
            leading=24,
            textColor=ACCENT_DARK,
            spaceAfter=10,
        ),
    }


class TechnicalDocTemplate(BaseDocTemplate):
    def __init__(self, filename: str, style_map: dict[str, ParagraphStyle]):
        super().__init__(
            filename,
            pagesize=A4,
            leftMargin=17 * mm,
            rightMargin=17 * mm,
            topMargin=19 * mm,
            bottomMargin=17 * mm,
            title="Clock Plus - технічна документація",
            author="Clock Plus project",
            subject="STM32L010F4P6 hardware and firmware technical reference",
        )
        self.style_map = style_map
        frame = Frame(
            self.leftMargin,
            self.bottomMargin,
            self.width,
            self.height,
            id="normal",
        )
        self.addPageTemplates(PageTemplate(id="all", frames=frame, onPage=self.page_art))

    def page_art(self, canvas, doc) -> None:
        page = canvas.getPageNumber()
        canvas.saveState()
        if page == 1:
            canvas.setFillColor(ACCENT_DARK)
            canvas.rect(0, A4[1] - 10 * mm, A4[0], 10 * mm, fill=1, stroke=0)
            canvas.setFillColor(ACCENT)
            canvas.rect(0, 0, A4[0], 8 * mm, fill=1, stroke=0)
            canvas.restoreState()
            return

        canvas.setStrokeColor(LINE)
        canvas.setLineWidth(0.45)
        canvas.line(self.leftMargin, A4[1] - 12 * mm, A4[0] - self.rightMargin, A4[1] - 12 * mm)
        canvas.setFont("TechSans", 7.5)
        canvas.setFillColor(MUTED)
        canvas.drawString(self.leftMargin, A4[1] - 9 * mm, "CLOCK PLUS / STM32L010F4P6")
        canvas.drawRightString(A4[0] - self.rightMargin, 9 * mm, f"Сторінка {page}")
        canvas.restoreState()

    def afterFlowable(self, flowable) -> None:
        if isinstance(flowable, Paragraph):
            style_name = flowable.style.name
            level = {"Heading1": 0, "Heading2": 1, "Heading3": 2}.get(style_name)
            if level is not None:
                text = flowable.getPlainText()
                key = f"heading-{self.seq.nextf('heading')}"
                self.canv.bookmarkPage(key)
                self.canv.addOutlineEntry(text, key, level=level, closed=False)
                self.notify("TOCEntry", (level, text, self.page, key))


def table_from_rows(rows: list[list[str]], style_map: dict[str, ParagraphStyle]):
    if not rows:
        return Spacer(1, 1)
    columns = max(len(row) for row in rows)
    normalized = [row + [""] * (columns - len(row)) for row in rows]
    available = A4[0] - 34 * mm

    weights = []
    for column in range(columns):
        longest = max(len(re.sub(r"`|\*", "", row[column])) for row in normalized)
        weights.append(max(8, min(longest, 36)))
    total = sum(weights)
    widths = [available * weight / total for weight in weights]

    data = []
    for row_index, row in enumerate(normalized):
        cell_style = style_map["table_header"] if row_index == 0 else style_map["table"]
        data.append([Paragraph(inline_markup(cell), cell_style) for cell in row])

    commands = [
        ("BACKGROUND", (0, 0), (-1, 0), ACCENT_DARK),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("GRID", (0, 0), (-1, -1), 0.35, LINE),
        ("LEFTPADDING", (0, 0), (-1, -1), 4),
        ("RIGHTPADDING", (0, 0), (-1, -1), 4),
        ("TOPPADDING", (0, 0), (-1, -1), 3.5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3.5),
    ]
    for row_index in range(1, len(data)):
        if row_index % 2 == 0:
            commands.append(("BACKGROUND", (0, row_index), (-1, row_index), TABLE_ALT))

    return Table(
        data,
        colWidths=widths,
        repeatRows=1,
        hAlign="LEFT",
        splitByRow=1,
        rowSplitRange=(2, -1),
        style=TableStyle(commands),
    )


def parse_markdown(source: str, style_map: dict[str, ParagraphStyle]):
    lines = source.splitlines()
    story = []
    index = 0
    paragraph_buffer: list[str] = []

    def flush_paragraph() -> None:
        if paragraph_buffer:
            text = " ".join(part.strip() for part in paragraph_buffer)
            story.append(Paragraph(inline_markup(text), style_map["body"]))
            paragraph_buffer.clear()

    while index < len(lines):
        line = lines[index]
        stripped = line.strip()

        if index < 18:
            index += 1
            continue

        if stripped.startswith("```"):
            flush_paragraph()
            code_lines = []
            index += 1
            while index < len(lines) and not lines[index].strip().startswith("```"):
                code_lines.append(lines[index])
                index += 1
            story.append(Preformatted("\n".join(code_lines), style_map["code"], maxLineLength=105))
            index += 1
            continue

        if stripped.startswith("|"):
            flush_paragraph()
            raw_rows = []
            while index < len(lines) and lines[index].strip().startswith("|"):
                raw_rows.append([cell.strip() for cell in lines[index].strip().strip("|").split("|")])
                index += 1
            rows = [
                row
                for row in raw_rows
                if not all(re.fullmatch(r":?-{3,}:?", cell.replace(" ", "")) for cell in row)
            ]
            story.append(KeepTogether([table_from_rows(rows, style_map)]))
            story.append(Spacer(1, 5))
            continue

        heading = re.match(r"^(#{2,4})\s+(.+)$", stripped)
        if heading:
            flush_paragraph()
            depth = len(heading.group(1))
            text = inline_markup(heading.group(2))
            style = style_map[{2: "h1", 3: "h2", 4: "h3"}[depth]]
            story.append(Paragraph(text, style))
            index += 1
            continue

        if stripped.startswith(">"):
            flush_paragraph()
            quote = stripped.lstrip("> ")
            story.append(Paragraph(inline_markup(quote), style_map["quote"]))
            index += 1
            continue

        bullet = re.match(r"^-\s+(.+)$", stripped)
        if bullet:
            flush_paragraph()
            story.append(Paragraph(inline_markup(bullet.group(1)), style_map["bullet"], bulletText="•"))
            index += 1
            continue

        numbered = re.match(r"^(\d+)\.\s+(.+)$", stripped)
        if numbered:
            flush_paragraph()
            story.append(
                Paragraph(
                    inline_markup(numbered.group(2)),
                    style_map["number"],
                    bulletText=f"{numbered.group(1)}.",
                )
            )
            index += 1
            continue

        if not stripped:
            flush_paragraph()
            index += 1
            continue

        paragraph_buffer.append(stripped)
        index += 1

    flush_paragraph()
    return story


def cover_story(style_map: dict[str, ParagraphStyle]):
    title = ParagraphStyle(
        "CoverTitle",
        fontName="TechSans-Bold",
        fontSize=28,
        leading=32,
        textColor=ACCENT_DARK,
        alignment=TA_LEFT,
        spaceAfter=8,
    )
    subtitle = ParagraphStyle(
        "CoverSubtitle",
        fontName="TechSans",
        fontSize=15,
        leading=20,
        textColor=MUTED,
        alignment=TA_LEFT,
    )
    label = ParagraphStyle(
        "CoverLabel",
        fontName="TechSans-Bold",
        fontSize=8,
        leading=11,
        textColor=ACCENT,
        alignment=TA_LEFT,
    )
    value = ParagraphStyle(
        "CoverValue",
        fontName="TechSans",
        fontSize=10,
        leading=14,
        textColor=INK,
        alignment=TA_LEFT,
    )

    metadata = Table(
        [
            [Paragraph("ПЛАТФОРМА", label), Paragraph("STM32L010F4P6 / Cortex-M0+", value)],
            [Paragraph("РЕДАКЦІЯ", label), Paragraph("Первинна технічна редакція", value)],
            [Paragraph("ЗВІРЕНО", label), Paragraph("2026-09-27, гілка optimizations", value)],
            [Paragraph("СКЛАД", label), Paragraph("Hardware, firmware, power states, build, KiCad appendices", value)],
        ],
        colWidths=[38 * mm, 115 * mm],
        style=TableStyle(
            [
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LINEBELOW", (0, 0), (-1, -2), 0.35, LINE),
                ("TOPPADDING", (0, 0), (-1, -1), 7),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
            ]
        ),
    )

    return [
        Spacer(1, 34 * mm),
        Paragraph("CLOCK PLUS", title),
        Paragraph("Технічна документація", subtitle),
        Spacer(1, 8 * mm),
        Table([[""]], colWidths=[42 * mm], rowHeights=[2.5 * mm], style=TableStyle([("BACKGROUND", (0, 0), (-1, -1), ACCENT)])),
        Spacer(1, 18 * mm),
        metadata,
        Spacer(1, 26 * mm),
        Paragraph(
            "Повний довідник апаратної частини та firmware з оригінальною "
            "принциповою схемою і комплектом шарів PCB.",
            style_map["body"],
        ),
        PageBreak(),
    ]


def build_body() -> None:
    register_fonts()
    style_map = styles()
    source = SOURCE_MD.read_text(encoding="utf-8")
    TMP_DIR.mkdir(parents=True, exist_ok=True)
    OUTPUT_PDF.parent.mkdir(parents=True, exist_ok=True)

    story = cover_story(style_map)
    story.append(Paragraph("Зміст", style_map["toc_title"]))
    toc = TableOfContents()
    toc.levelStyles = [
        ParagraphStyle(
            "TOC0",
            fontName="TechSans-Bold",
            fontSize=9,
            leading=12,
            leftIndent=0,
            firstLineIndent=0,
            textColor=ACCENT_DARK,
            spaceBefore=3,
        ),
        ParagraphStyle(
            "TOC1",
            fontName="TechSans",
            fontSize=8.2,
            leading=11,
            leftIndent=11,
            firstLineIndent=0,
            textColor=INK,
        ),
        ParagraphStyle(
            "TOC2",
            fontName="TechSans",
            fontSize=7.6,
            leading=10,
            leftIndent=22,
            firstLineIndent=0,
            textColor=MUTED,
        ),
    ]
    story.extend([toc, PageBreak()])
    story.extend(parse_markdown(source, style_map))
    story.extend(
        [
            PageBreak(),
            Paragraph("Технічні додатки KiCad", style_map["h1"]),
            Paragraph(
                "Після цієї сторінки без змін додано оригінальний PDF принципової "
                "схеми та дев'ять сторінок PCB output. Вони є частиною технічного "
                "пакета, але в разі розбіжності поведінку поточної прошивки визначає "
                "вихідний код і таблиця перевірки у розділі 5.3.",
                style_map["body"],
            ),
        ]
    )

    document = TechnicalDocTemplate(str(BODY_PDF), style_map)
    document.multiBuild(story)


def merge_appendices() -> None:
    writer = PdfWriter()
    body_reader = PdfReader(str(BODY_PDF))
    body_page_text = [(page.extract_text() or "") for page in body_reader.pages]

    for path in (BODY_PDF, SCHEMATIC_PDF, PCB_PDF):
        reader = PdfReader(str(path))
        for page in reader.pages:
            writer.add_page(page)

    for line in SOURCE_MD.read_text(encoding="utf-8").splitlines():
        match = re.match(r"^##\s+(.+)$", line.strip())
        if match is None or match.group(1) == "Технічна документація апаратної частини та firmware":
            continue
        title = re.sub(r"`|\*", "", match.group(1))
        for page_index, page_text in enumerate(body_page_text):
            if title in page_text:
                writer.add_outline_item(title, page_index)
                break

    appendix_page = len(body_reader.pages) - 1
    writer.add_outline_item("Технічні додатки KiCad", appendix_page)
    writer.add_outline_item("Додаток A. Принципова схема", len(body_reader.pages))
    writer.add_outline_item("Додаток B. Комплект шарів PCB", len(body_reader.pages) + 1)

    writer.add_metadata(
        {
            "/Title": "Clock Plus - технічна документація",
            "/Author": "Clock Plus project",
            "/Subject": "STM32L010F4P6 hardware and firmware technical reference",
            "/Keywords": "STM32L010F4P6, RTC, 74HC595, AHT10, BH1750, KiCad",
        }
    )
    with OUTPUT_PDF.open("wb") as stream:
        writer.write(stream)


def main() -> None:
    for required in (SOURCE_MD, SCHEMATIC_PDF, PCB_PDF):
        if not required.exists():
            raise FileNotFoundError(required)
    build_body()
    merge_appendices()
    print(OUTPUT_PDF)


if __name__ == "__main__":
    main()
