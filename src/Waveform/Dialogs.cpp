#include "stdafx.h"

#include "WaveformWindow.h"

INT_PTR CALLBACK NewWaveDialogProc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (wave == NULL)
		{
			EndDialog(dlg, NULL);
			return TRUE;
		}

		SetDlgItemInt(dlg, IDC_CHANNELS, newWave.format.channels, FALSE);
		SetDlgItemInt(dlg, IDC_SAMPLE_RATE, newWave.format.samplesPerSecond, FALSE);
		SetDlgItemInt(dlg, IDC_SECOND_LENGTH, newWave.seconds, FALSE);

		HWND cmbCalcMethod = GetDlgItem(dlg, IDC_BITS);
		SendMessage(cmbCalcMethod, CB_ADDSTRING, NULL, (LPARAM)_T("8"));
		SendMessage(cmbCalcMethod, CB_ADDSTRING, NULL, (LPARAM)_T("16"));
		SendMessage(cmbCalcMethod, CB_ADDSTRING, NULL, (LPARAM)_T("32"));
		SendMessage(cmbCalcMethod, CB_ADDSTRING, NULL, (LPARAM)_T("64"));

		int selIndex = 0;
		switch (newWave.format.bitsPerSample)
		{
		default:
		case 8: selIndex = 0; break;
		case 16: selIndex = 1; break;
		case 32: selIndex = 2; break;
		case 64: selIndex = 3; break;
		}

		SendMessage(cmbCalcMethod, CB_SETCURSEL, selIndex, NULL);
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			newWave.format.channels = GetDlgItemInt(dlg, IDC_CHANNELS, FALSE, FALSE);
			newWave.format.samplesPerSecond = GetDlgItemInt(dlg, IDC_SAMPLE_RATE, FALSE, FALSE);
			newWave.seconds = GetDlgItemInt(dlg, IDC_SECOND_LENGTH, FALSE, FALSE);

			{
				int i = SendMessage(GetDlgItem(dlg, IDC_BITS), CB_GETCURSEL, NULL, NULL);
				switch (i)
				{
				default: case CB_ERR:
				case 0: newWave.format.bitsPerSample = 8; break;
				case 1: newWave.format.bitsPerSample = 16; break;
				case 2: newWave.format.bitsPerSample = 32; break;
				case 3: newWave.format.bitsPerSample = 64; break;
				}
			}

		case IDCANCEL:
			EndDialog(dlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK GenerateWaveDialogProc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetDlgItemInt(dlg, IDC_FREQ, genWave.freq, FALSE);
		SetDlgItemInt(dlg, IDC_VOLUME, genWave.volume, TRUE);
		SetDlgItemInt(dlg, IDC_PHASE_DEGREE, genWave.phase, TRUE);
		SetDlgItemInt(dlg, IDC_SEGMENT, genWave.seg, TRUE);

		HWND cmbCalcMethod = GetDlgItem(dlg, IDC_CALC_METHOD);
		SendMessage(cmbCalcMethod, CB_ADDSTRING, NULL, (LPARAM)_T("Set"));
		SendMessage(cmbCalcMethod, CB_ADDSTRING, NULL, (LPARAM)_T("Add"));
		SendMessage(cmbCalcMethod, CB_ADDSTRING, NULL, (LPARAM)_T("Sub"));

		int selIndex = 0;
		switch (genWave.flags)
		{
		default:
		case GenerateFlag::Set: selIndex = 0; break;
		case GenerateFlag::Add: selIndex = 1; break;
		case GenerateFlag::Sub: selIndex = 2; break;
		}
		SendMessage(cmbCalcMethod, CB_SETCURSEL, selIndex, NULL);

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			uint frequency = (uint)GetDlgItemInt(dlg, IDC_FREQ, FALSE, FALSE);
			if (wave->format.samplesPerSecond <= frequency * 2)
			{
				MessageBox(dlg, _T("Sampling rate of this waveform is too low to store specified frequency."),
					_T("Waveform Box"), MB_OK);
				return TRUE;
			}

			genWave.freq = GetDlgItemInt(dlg, IDC_FREQ, FALSE, FALSE);
			genWave.volume = GetDlgItemInt(dlg, IDC_VOLUME, FALSE, TRUE);
			genWave.phase = GetDlgItemInt(dlg, IDC_PHASE_DEGREE, FALSE, TRUE);
			genWave.seg = GetDlgItemInt(dlg, IDC_SEGMENT, FALSE, TRUE);

			int i = SendMessage(GetDlgItem(dlg, IDC_CALC_METHOD), CB_GETCURSEL, NULL, NULL);
			switch (i)
			{
			default: case CB_ERR:
			case 0: genWave.flags = GenerateFlag::Set; break;
			case 1: genWave.flags = GenerateFlag::Add; break;
			case 2: genWave.flags = GenerateFlag::Sub; break;
			}
		}

		case IDCANCEL:
			EndDialog(dlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK VolumeDialogProc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR tmp[32];

	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetDlgItemInt(dlg, IDC_VOLUME, vol.volumePercent, TRUE);

		_stprintf_s(tmp, _T("%.2lf"), vol.fadeInSeconds);
		SetDlgItemText(dlg, IDC_FADE_IN, tmp);

		_stprintf_s(tmp, _T("%.2lf"), vol.fadeOutSeconds);
		SetDlgItemText(dlg, IDC_FADE_OUT, tmp);

		CheckDlgButton(dlg, IDC_APPLY_TO_SELECTION, vol.applyToSelection);
		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			vol.volumePercent = GetDlgItemInt(dlg, IDC_VOLUME, FALSE, TRUE);

			GetDlgItemText(dlg, IDC_FADE_IN, tmp, 32);
			_stscanf_s(tmp, _T("%lf"), &vol.fadeInSeconds);

			GetDlgItemText(dlg, IDC_FADE_OUT, tmp, 32);
			_stscanf_s(tmp, _T("%lf"), &vol.fadeOutSeconds);

			vol.applyToSelection = IsDlgButtonChecked(dlg, IDC_APPLY_TO_SELECTION) == TRUE;

		case IDCANCEL:
			EndDialog(dlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return (INT_PTR)FALSE;
}
