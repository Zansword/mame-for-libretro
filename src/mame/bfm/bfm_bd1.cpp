// license:BSD-3-Clause
// copyright-holders:James Wallace
/**********************************************************************

    Bellfruit BD1 VFD module interface and emulation

    TODO: Verify flashing (our only datasheet has that section
    completely illegible)

    This is a simulation of code running on an NEC D78042GF-090
**********************************************************************/

#include "emu.h"
#include "bfm_bd1.h"

DEFINE_DEVICE_TYPE(BFM_BD1, bfm_bd1_device, "bfm_bd1", "BFM BD1 VFD controller")


/*
   BD1 14 segment charset lookup table, according to datasheet (we rewire this later)

        2
    ---------
   |\   |3  /|
 0 | \6 |  /7| 1
   |  \ | /  |
    -F-- --E-
   |  / | \  |
 D | /4 |A \B| 5
   |/   |   \|
    ---------  C
        9

        8 is flashing

  */

static const uint16_t BD1charset[]=
{           // FEDC BA98 7654 3210
	0xA626, // 1010 0110 0010 0110 @.
	0xE027, // 1110 0000 0010 0111 A.
	0x462E, // 0100 0110 0010 1110 B.
	0x2205, // 0010 0010 0000 0101 C.
	0x062E, // 0000 0110 0010 1110 D.
	0xA205, // 1010 0010 0000 0101 E.
	0xA005, // 1010 0000 0000 0101 F.
	0x6225, // 0110 0010 0010 0101 G.
	0xE023, // 1110 0000 0010 0011 H.
	0x060C, // 0000 0110 0000 1100 I.
	0x2222, // 0010 0010 0010 0010 J.
	0xA881, // 1010 1000 1000 0001 K.
	0x2201, // 0010 0010 0000 0001 L.
	0x20E3, // 0010 0000 1110 0011 M.
	0x2863, // 0010 1000 0110 0011 N.
	0x2227, // 0010 0010 0010 0111 O.
	0xE007, // 1110 0000 0000 0111 P.
	0x2A27, // 0010 1010 0010 0111 Q.
	0xE807, // 1110 1000 0000 0111 R.
	0xC225, // 1100 0010 0010 0101 S.
	0x040C, // 0000 0100 0000 1100 T.
	0x2223, // 0010 0010 0010 0011 U.
	0x2091, // 0010 0000 1001 0001 V.
	0x2833, // 0010 1000 0011 0011 W.
	0x08D0, // 0000 1000 1101 0000 X.
	0x04C0, // 0000 0100 1100 0000 Y.
	0x0294, // 0000 0010 1001 0100 Z.
	0x2205, // 0010 0010 0000 0101 [.
	0x0840, // 0000 1000 0100 0000 \.
	0x0226, // 0000 0010 0010 0110 ].
	0x0810, // 0000 1000 0001 0000 ^.
	0x0200, // 0000 0010 0000 0000 _
	0x0000, // 0000 0000 0000 0000
	0xC290, // 1100 0010 1001 0000 POUND.
	0x0009, // 0000 0000 0000 1001 ".
	0xC62A, // 1100 0110 0010 1010 #.
	0xC62D, // 1100 0110 0010 1101 $.
	0x0100, // 0000 0001 0000 0000 flash character
	0x0000, // 0000 0000 0000 0000 not defined
	0x0080, // 0000 0000 1000 0000 '.
	0x0880, // 0000 1000 1000 0000 (.
	0x0050, // 0000 0000 0101 0000 ).
	0xCCD8, // 1100 1100 1101 1000 *.
	0xC408, // 1100 0100 0000 1000 +.
	0x1000, // 0001 0000 0000 0000 .
	0xC000, // 1100 0000 0000 0000 -.
	0x1000, // 0001 0000 0000 0000 ..
	0x0090, // 0000 0000 1001 0000 /.
	0x22B7, // 0010 0010 1011 0111 0.
	0x0408, // 0000 0100 0000 1000 1.
	0xE206, // 1110 0010 0000 0110 2.
	0xC226, // 0100 0010 0010 0110 3.
	0xC023, // 1100 0000 0010 0011 4.
	0xC225, // 1100 0010 0010 0101 5.
	0xE225, // 1110 0010 0010 0101 6.
	0x0026, // 0000 0000 0010 0110 7.
	0xE227, // 1110 0010 0010 0111 8.
	0xC227, // 1100 0010 0010 0111 9.
	0xFFFF, // 0000 0000 0000 0000 user defined, can be replaced by main program
	0x0000, // 0000 0000 0000 0000 dummy
	0x0290, // 0000 0010 1001 0000 <.
	0xC200, // 1100 0010 0000 0000 =.
	0x0A40, // 0000 1010 0100 0000 >.
	0x4406, // 0100 0100 0000 0110 ?
};

bfm_bd1_device::bfm_bd1_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, BFM_BD1, tag, owner, clock)
	, m_outputs()
	, m_port_val(0)
{
}

void bfm_bd1_device::device_start()
{
	m_outputs = std::make_unique<output_finder<16> >(*this, "vfd%u", unsigned(m_port_val) << 4);
	m_outputs->resolve();

	save_item(NAME(m_cursor_pos));
	save_item(NAME(m_window_start));        // display window start pos 0-15
	save_item(NAME(m_window_end));      // display window end   pos 0-15
	save_item(NAME(m_window_size));     // window  size
	save_item(NAME(m_shift_count));
	save_item(NAME(m_shift_data));
	save_item(NAME(m_pcursor_pos));
	save_item(NAME(m_scroll_active));
	save_item(NAME(m_display_mode));
	save_item(NAME(m_flash_rate));
	save_item(NAME(m_flash_control));
	save_item(NAME(m_sclk));
	save_item(NAME(m_data));

	save_item(NAME(m_cursor));
	save_item(NAME(m_chars));
	save_item(NAME(m_attrs));
	save_item(NAME(m_user_data));           // user defined character data (16 bit)
	save_item(NAME(m_user_def));            // user defined character state
}

void bfm_bd1_device::device_reset()
{
	m_cursor = 0;
	m_cursor_pos = 0;
	m_window_start = 0;
	m_window_end = 0;
	m_window_size = 0;
	m_shift_count = 0;
	m_shift_data = 0;
	m_pcursor_pos = 0;
	m_scroll_active = false;
	m_display_mode = 0;
	m_flash_rate = 0;
	m_flash_control = 0;
	m_flash = false;
	m_flash_timer = 0;
	m_user_data = 0;
	m_user_def = 0;
	m_sclk = 0;
	m_data = 0;

	std::fill(std::begin(m_chars), std::end(m_chars), 0);
	std::fill(std::begin(m_attrs), std::end(m_attrs), 0);
}

uint16_t bfm_bd1_device::set_display(uint16_t segin)
{
	return bitswap<16>(segin,8,12,11,7,6,4,10,3,14,15,0,13,9,5,1,2);
}

void bfm_bd1_device::device_post_load()
{
	update_display();
}

void bfm_bd1_device::update_display()
{
	if (m_flash_timer)
	{
		m_flash_timer--;
		if (!m_flash_timer)
		{
			m_flash_timer = 20;
			if (!m_flash)
			{
				switch (m_flash_control)
				{
				case 1:    // Flash Inside Window
					for (int i = 0; i < 16; i++)
					{
						if ((i >= m_window_start) && (i <= m_window_end))
							m_attrs[i] = AT_FLASH;
						else
							m_attrs[i] = AT_NORMAL;
					}
					m_flash = true;
					break;
				case 2:    // Flash Outside Window
					for (int i = 0; i < 16; i++)
					{
					  if ((i < m_window_start) || (i > m_window_end))
						  m_attrs[i] = AT_FLASH;
					  else
						  m_attrs[i] = AT_NORMAL;
					}
					m_flash = true;
					break;
				case 3:    // Flash All
					for ( int i = 0; i < 16; i++ )
						m_attrs[i] = AT_FLASH;
					m_flash = true;
					break;
				}
		  }
		  else
		  {
			m_flash_rate--;
			if (!m_flash_rate)
			{
				m_flash_timer = 0;
				for (int i = 0; i < 16; i++)
				{
					m_attrs[i] = AT_NORMAL;
				}
			}
			if (m_flash_control)
			{
				m_flash = false;
			}
		  }
		}
	}

	for (int i = 0; i < 16; i++)
		(*m_outputs)[i] = (m_attrs[i] == AT_NORMAL) ? set_display(m_chars[i]) : 0;
}
///////////////////////////////////////////////////////////////////////////
void bfm_bd1_device::blank(int data)
{
	switch ( data & 0x03 )
	{
	case 0x00:  //blank all
		for (int i = 0; i < 15; i++)
		{
			m_attrs[i] = AT_BLANK;
		}
		break;

	case 0x01:  // blank inside window
		if (m_window_size > 0)
		{
			for (int i = m_window_start; i < m_window_end ; i++)
			{
				m_attrs[i] = AT_BLANK;
			}
		}
		break;

	case 0x02:  // blank outside window
		if (m_window_size > 0)
		{
			if (m_window_start > 0)
			{
				for (int i = 0; i < m_window_start; i++)
				{
					m_attrs[i] = AT_BLANK;
				}
			}

			if (m_window_end < 15 )
			{
				for (int i = m_window_end; i < 15- m_window_end ; i++)
				{
					m_attrs[i] = AT_BLANK;
				}
			}
		}
		break;

	case 0x03:  // clear blanking
		for (int i = 0; i < 15; i++)
		{
			m_attrs[i] = 0;
		}
		break;
	}
}

int bfm_bd1_device::write_char(int data)
{
	if (m_user_def)
	{
		m_user_def--;

		m_user_data <<= 8;
		m_user_data |= data;

		if (m_user_def)
		{
			return 0;
		}

		setdata(m_user_data, data);
	}
	else
	{
		if(data < 0x80)//characters
		{
			if (data > 0x3f)
			{
				logerror("Undefined character %x \n", data);
			}

			setdata(BD1charset[(data & 0x3f)], data);
		}
		else
		{
			switch (data & 0xf0)
			{
			case 0x80:  // 0x80 - 0x8F Set display blanking
				blank(data&0x03);//use the blanking data

				if (data == 0x84)
				{
					popmessage("Duty control active, contact MAMEDEV");
				}
				break;

			case 0x90:  // 0x90 - 0x9F Set cursor pos
				m_cursor_pos = data & 0x0f;
				m_scroll_active = false;
				if (m_display_mode == 2)
				{
					if (m_cursor_pos >= m_window_end) m_scroll_active = 1;
				}
				break;

			case 0xa0:  // 0xA0 - 0xAF Set display mode
				m_display_mode = data &0x03;
				break;

			case 0xb0:  // 0xB0 - 0xBF Clear display area

				switch (data & 0x03)
				{
				case 0x00:  // clr nothing
					break;

				case 0x01:  // clr inside window
					if (m_window_size > 0)
					{
						std::fill_n(m_chars + m_window_start, m_window_size, 0);
						std::fill_n(m_attrs + m_window_start, m_window_size, 0);
					}

					break;

				case 0x02:  // clr outside window
					if (m_window_size > 0)
					{
						if (m_window_start > 0)
						{
							for (int i = 0; i < m_window_start; i++)
							{
								memset(m_chars+i,0,i);
								memset(m_attrs+i,0,i);
							}
						}

						if (m_window_end < 15)
						{
							for (int i = m_window_end; i < 15- m_window_end ; i++)
							{
								memset(m_chars+i,0,i);
								memset(m_attrs+i,0,i);
							}
						}
					}
					break;

				case 0x03:  // clr entire display
					std::fill(std::begin(m_chars), std::end(m_chars), 0);
					std::fill(std::begin(m_attrs), std::end(m_attrs), 0);
				}
				break;

			case 0xc0:  // 0xC0 - 0xCF Set flash rate
				m_flash_rate = data & 0x0f;
				if (!m_flash_rate && m_flash)
				{
					m_flash = false;
				}
				m_flash_timer = 20;
				break;

			case 0xd0:  // 0xD0 - 0xDF Set Flash control
				m_flash_control = data & 0x03;
				if (m_flash_control == 0 && m_flash)
				{
					m_flash = false;
				}
				break;

			case 0xe0:  // 0xE0 - 0xEF Set window start pos
				m_window_start = data &0x0f;
				m_window_size  = (m_window_end - m_window_start)+1;
				break;

			case 0xf0:  // 0xF0 - 0xFF Set window end pos
				m_window_end   = data &0x0f;
				m_window_size  = (m_window_end - m_window_start)+1;
				m_scroll_active = 0;
				if (m_display_mode == 2)
				{
					if (m_cursor_pos >= m_window_end)
					{
						m_scroll_active = 1;
						m_cursor_pos    = m_window_end;
					}
				}
				break;

			default:
				popmessage("%x",data);
			}
		}
	}
	update_display();

	return 0;
}
///////////////////////////////////////////////////////////////////////////

void bfm_bd1_device::setdata(int segdata, int data)
{
	int move = 0;
	int change =0;
	switch (data)
	{
	case 0x25:  // flash
		if(m_chars[m_pcursor_pos] & (1<<8))
		{
			move++;
		}
		else
		{
			m_attrs[m_pcursor_pos] = AT_FLASH;
			m_chars[m_pcursor_pos] |= (1<<8);
		}
		break;

	case 0x26:  // undefined
		break;

	case 0x2c:  // semicolon
	case 0x2e:  // decimal point
		if (m_chars[m_pcursor_pos] & (1<<12))
		{
			move++;
		}
		else
		{
			m_chars[m_pcursor_pos] |= (1<<12);
		}
		break;

	case 0x3a:
		m_user_def = 2;
		break;

	case 0x3b:  // dummy char
		move++;
		break;


	default:
		move++;
		change++;
	}

	if (move)
	{
		int mode = m_display_mode;

		m_pcursor_pos = m_cursor_pos;

		if ( m_window_size <= 0 || (m_window_size > 16))
		{ // if no window selected default to equivalent rotate mode
				if (mode == 2)      mode = 0;
				else if (mode == 3) mode = 1;
		}

		switch (mode)
		{
		case 0: // rotate left
			m_cursor_pos &= 0x0f;

			if (change)
			{
				m_chars[m_cursor_pos] = segdata;
			}
			m_cursor_pos++;
			if (m_cursor_pos >= 16) m_cursor_pos = 0;
			break;


		case 1: // Rotate right
			m_cursor_pos &= 0x0f;

			if (change)
			{
				m_chars[m_cursor_pos] = segdata;
			}
			m_cursor_pos--;
			if (m_cursor_pos < 0) m_cursor_pos = 15;
			break;

		case 2: // Scroll left
			if ( m_cursor_pos < m_window_end )
			{
				m_scroll_active = 0;
				if (change)
				{
					m_chars[m_cursor_pos] = segdata;
				}
				m_cursor_pos++;
			}
			else
			{
				if (move)
				{
					if  (m_scroll_active)
					{
						int i = m_window_start;
						while (i < m_window_end)
						{
							m_chars[i] = m_chars[i+1];
							i++;
						}
					}
					else   m_scroll_active = 1;
				}

				if (change)
				{
					m_chars[m_window_end] = segdata;
				}
				else
				{
					m_chars[m_window_end] = 0;
				}
			}
			break;

		case 3: // Scroll right
			if (m_cursor_pos > m_window_start)
			{
				if (change)
				{
					m_chars[m_cursor_pos] = segdata;
				}
				m_cursor_pos--;
				if (m_cursor_pos > 15) m_cursor_pos = 0;
			}
			else
			{
				if (move)
				{
					if (m_scroll_active)
					{
						int i = m_window_end;

						while (i > m_window_start)
						{
							m_chars[i] = m_chars[i-1];
							i--;
						}
					}
					else   m_scroll_active = 1;
				}
				if (change)
				{
					m_chars[m_window_start] = segdata;
				}
				else
				{
					m_chars[m_window_start] = 0;
				}
			}
			break;
		}
	}
}

void bfm_bd1_device::shift_clock(int state)
{
	if (m_sclk != state)
	{
		if (!m_sclk)
		{
			m_shift_data <<= 1;

			if ( !m_data ) m_shift_data |= 1;

			if ( ++m_shift_count >= 8 )
			{
				write_char(m_shift_data);
				m_shift_count = 0;
				m_shift_data  = 0;
			}
			update_display();

		}
	}
	m_sclk = state;
}

WRITE_LINE_MEMBER( bfm_bd1_device::sclk ) { shift_clock(state); }
WRITE_LINE_MEMBER( bfm_bd1_device::data ) { m_data = state; }

WRITE_LINE_MEMBER( bfm_bd1_device::por )
{
	if (!state)
	{
		reset();
	}
}
