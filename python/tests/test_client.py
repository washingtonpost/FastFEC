import datetime
import os

from fastfec import FastFEC


def test_filing_1550126_line_callback(filing_1550126):
    """
    Test the FastFEC line-by-line callback functionality, in addition
    to the package's ability to parse summary, contribution and
    disbursement rows.
    """
    with open(filing_1550126, "rb") as f:
        with FastFEC() as fastfec:
            parsed = list(fastfec.parse(f))

            # Test the summary data parse
            summary_form, summary_data = parsed[1]
            assert len(summary_data.keys()) == 93
            assert summary_form == "F3A"
            assert summary_data["filer_committee_id_number"] == "C00772335"
            assert (
                summary_data["committee_name"] == "Jeffrey Buongiorno for US Congress"
            )
            assert summary_data["election_date"] == ""
            assert summary_data["coverage_from_date"] == datetime.date(2021, 7, 1)
            assert summary_data["col_a_total_contributions_no_loans"] == 4239.0
            assert summary_data["col_b_total_disbursements"] == 9229.09

            # Test the contribution data parse
            contribution_form, contribution_data = parsed[2]
            assert len(contribution_data.keys()) == 45
            assert contribution_form == "SA11AI"
            assert contribution_data["filer_committee_id_number"] == "C00772335"
            assert contribution_data["transaction_id"] == "SA11AI.4265"
            assert contribution_data["entity_type"] == "IND"
            assert contribution_data["contributor_last_name"] == "barbariniweil"
            assert contribution_data["contributor_first_name"] == "dale"
            assert contribution_data["contribution_date"] == datetime.date(2021, 8, 5)
            assert contribution_data["contribution_amount"] == 1000.0
            assert contribution_data["reference_code"] is None

            # Test the disbursement data parse
            disbursement_form, disbursement_data = parsed[8]
            print("disbursement_data", disbursement_data)
            assert len(disbursement_data.keys()) == 44
            assert disbursement_form == "SB17"


def test_filing_1550548_parse_as_files(tmpdir, filing_1550548):
    """
    Test that the FastFEC `parse_as_files` method outputs the correct files
    and that they are the correct length.
    """
    with open(filing_1550548, "rb") as f:
        with FastFEC() as fastfec:
            fastfec.parse_as_files(f, tmpdir)

    assert (
        os.listdir(tmpdir).sort()
        == [
            "SB23.csv",
            "header.csv",
            "SB21B.csv",
            "F3XA.csv",
            "SA11AI.csv",
        ].sort()
    )

    with open(os.path.join(tmpdir, "header.csv")) as f:
        assert len(f.readlines()) == 2

    with open(os.path.join(tmpdir, "F3XA.csv")) as f:
        assert len(f.readlines()) == 2

    with open(os.path.join(tmpdir, "SA11AI.csv")) as f:
        assert len(f.readlines()) == 77

    with open(os.path.join(tmpdir, "SB21B.csv")) as f:
        assert len(f.readlines()) == 8

    with open(os.path.join(tmpdir, "SB23.csv")) as f:
        assert len(f.readlines()) == 36
