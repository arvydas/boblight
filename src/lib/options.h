/*
 * boblight
 * Copyright (C) Bob  2009 
 * 
 * boblight is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * boblight is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//              name           type   min              max                default variable
BOBLIGHT_OPTION(speed,         float, 0.0,             100.0,             100.0,  m_speed)
BOBLIGHT_OPTION(autospeed,     bool,  -1,              -1,                0,      m_autospeed)
BOBLIGHT_OPTION(interpolation, bool,  -1,              -1,                0,      m_interpolation)
BOBLIGHT_OPTION(use,           bool,  -1,              -1,                0,      m_use)
BOBLIGHT_OPTION(saturation,    float, 0.0,             -1,                1.0,    m_saturation)
BOBLIGHT_OPTION(value,         float, 0.0,             -1,                1.0,    m_value)
BOBLIGHT_OPTION(valuemin,      float, 0.0,             m_valuerange[1],   0.0,    m_valuerange[0])
BOBLIGHT_OPTION(valuemax,      float, m_valuerange[0], 1.0,               1.0,    m_valuerange[1])
BOBLIGHT_OPTION(threshold,     int,   0,               255,               0,      m_threshold)
